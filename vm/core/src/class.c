/*
 * Copyright (C) 2012 RoboVM
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <robovm.h>
#include <hythread.h>
#include <stdlib.h>
#include <string.h>
#include "utlist.h"
#include "private.h"
#include "uthash.h"

#define LOG_TAG "core.class"

Class* java_lang_Object;
Class* java_lang_Class;
Class* java_lang_ClassLoader;
Class* java_lang_String;
Class* java_lang_Boolean;
Class* java_lang_Byte;
Class* java_lang_Character;
Class* java_lang_Short;
Class* java_lang_Integer;
Class* java_lang_Long;
Class* java_lang_Float;
Class* java_lang_Double;
Class* java_lang_Enum;
Class* java_lang_Cloneable;
Class* java_lang_Thread;
Class* java_lang_Runtime;
Class* java_lang_ThreadGroup;
Class* java_io_Serializable;

Class* java_lang_Error;
Class* java_lang_OutOfMemoryError;
Class* java_lang_NoClassDefFoundError;
Class* java_lang_IllegalAccessError;
Class* java_lang_NoSuchFieldError;
Class* java_lang_NoSuchMethodError;
Class* java_lang_IncompatibleClassChangeError;
Class* java_lang_AbstractMethodError;
Class* java_lang_UnsatisfiedLinkError;
Class* java_lang_ExceptionInInitializerError;
Class* java_lang_VerifyError;
Class* java_lang_LinkageError;
Class* java_lang_InstantiationError;

Class* java_lang_Throwable;
Class* java_lang_RuntimeException;
Class* java_lang_ClassCastException;
Class* java_lang_NullPointerException;
Class* java_lang_ArrayIndexOutOfBoundsException;
Class* java_lang_ArrayStoreException;
Class* java_lang_ClassNotFoundException;
Class* java_lang_NegativeArraySizeException;
Class* java_lang_IllegalArgumentException;
Class* java_lang_ArithmeticException;
Class* java_lang_UnsupportedOperationException;
Class* java_lang_IllegalMonitorStateException;
Class* java_lang_InstantiationException;
Class* java_lang_InterruptedException;

Class* java_lang_InterruptedException;

Class* prim_Z;
Class* prim_B;
Class* prim_C;
Class* prim_S;
Class* prim_I;
Class* prim_J;
Class* prim_F;
Class* prim_D;
Class* prim_V;

Class* array_Z;
Class* array_B;
Class* array_C;
Class* array_S;
Class* array_I;
Class* array_J;
Class* array_F;
Class* array_D;

static Field FIELDS_NOT_LOADED = {0};
static Method METHODS_NOT_LOADED = {0};
static Interface INTERFACES_NOT_LOADED = {0};
static Boolean* java_lang_Boolean_TRUE = NULL;
static Boolean* java_lang_Boolean_FALSE = NULL;
static Method* java_lang_Byte_valueOf = NULL;
static ObjectArray* bytesCache = NULL;
static Method* java_lang_Short_valueOf = NULL;
static ObjectArray* shortsCache = NULL;
static Method* java_lang_Character_valueOf = NULL;
static ObjectArray* charactersCache = NULL;
static Method* java_lang_Integer_valueOf = NULL;
static ObjectArray* integersCache = NULL;
static Method* java_lang_Long_valueOf = NULL;
static ObjectArray* longsCache = NULL;
static Method* java_lang_Float_valueOf = NULL;
static Method* java_lang_Double_valueOf = NULL;

static hythread_monitor_t classLock;

typedef struct LoadedClassEntry {
    const char* key;      // The class name
    Class* clazz;
    UT_hash_handle hh;
} LoadedClassEntry;
static LoadedClassEntry* loadedClasses = NULL;

static jint nextClassId = 0;

static Class* findClassByDescriptor(Env* env, const char* desc, ClassLoader* classLoader, Class* (*loaderFunc)(Env*, const char*, ClassLoader*));
static Class* findClass(Env* env, const char* className, ClassLoader* classLoader, Class* (*loaderFunc)(Env*, const char*, ClassLoader*));
static Class* findBootClass(Env* env, const char* className);

static inline jint getNextClassId(void) {
    return __sync_fetch_and_add(&nextClassId, 1);
}

static Class* getLoadedClass(Env* env, const char* className) {
    LoadedClassEntry* entry;
    HASH_FIND_STR(loadedClasses, className, entry);
    return entry ? entry->clazz : NULL;
}

static jboolean addLoadedClass(Env* env, Class* clazz) {
    LoadedClassEntry* entry = rvmAllocateMemory(env, sizeof(LoadedClassEntry));
    if (!entry) return FALSE;
    entry->key = clazz->name;
    entry->clazz = clazz;
    HASH_ADD_KEYPTR(hh, loadedClasses, entry->key, strlen(entry->key), entry);
    return TRUE;
}

static inline void obtainClassLock() {
    hythread_monitor_enter(classLock);
}

static inline void releaseClassLock() {
    hythread_monitor_exit(classLock);
}

static Class* createPrimitiveClass(Env* env, const char* desc) {
    Class* clazz = rvmAllocateMemory(env, sizeof(Class));
    if (!clazz) return NULL;
    clazz->name = desc;
    clazz->object.clazz = java_lang_Class;
    clazz->flags = CLASS_FLAG_PRIMITIVE | CLASS_STATE_INITIALIZED | ACC_PUBLIC | ACC_FINAL | ACC_ABSTRACT;
    return clazz;
}

static Class* createArrayClass(Env* env, Class* componentType) {
    jint length = strlen(componentType->name);
    char* desc = NULL;

    if (CLASS_IS_ARRAY(componentType) || CLASS_IS_PRIMITIVE(componentType)) {
        desc = rvmAllocateMemory(env, length + 2);
        if (!desc) return NULL;
        desc[0] = '[';
        strcat(desc, componentType->name);
    } else {
        desc = rvmAllocateMemory(env, length + 4);
        if (!desc) return NULL;
        desc[0] = '[';
        desc[1] = 'L';
        strcat(desc, componentType->name);
        desc[length + 2] = ';';
    }

    // TODO: Add clone() method.
    Class* clazz = rvmAllocateClass(env, desc, java_lang_Object, componentType->classLoader, 
        CLASS_FLAG_ARRAY | CLASS_STATE_INITIALIZED | ACC_PUBLIC | ACC_FINAL | ACC_ABSTRACT, sizeof(Class), sizeof(Object), sizeof(Object), NULL, NULL);
    if (!clazz) return NULL;
    clazz->componentType = componentType;
    // Initialize methods to NULL to prevent rvmGetMethods() from trying to load the methods if called with this array class
    clazz->_methods = NULL;
    if (!rvmAddInterface(env, clazz, java_lang_Cloneable)) return NULL;
    if (!rvmAddInterface(env, clazz, java_io_Serializable)) return NULL;
    if (!rvmRegisterClass(env, clazz)) return NULL;

    return clazz;
}

static Class* findClass(Env* env, const char* className, ClassLoader* classLoader, Class* (*loaderFunc)(Env*, const char*, ClassLoader*)) {
    obtainClassLock();
    Class* clazz = getLoadedClass(env, className);
    if (clazz != NULL) {
        releaseClassLock();
        return clazz;
    }

    if (className[0] == '[') {
        Class* componentType = findClassByDescriptor(env, &className[1], classLoader, loaderFunc);
        if (!componentType)  {
            releaseClassLock();
            return NULL;
        }
        clazz = createArrayClass(env, componentType);
        releaseClassLock();
        return clazz;
    }

    TRACEF("Class '%s' not loaded", className);

    clazz = loaderFunc(env, className, classLoader);
    if (rvmExceptionOccurred(env)) {
        releaseClassLock();
        return NULL;
    }

    if (clazz == NULL) {
        if (!strcmp(className, "java/lang/ClassNotFoundException")) {
            rvmAbort("Fatal error: java.lang.ClassNotFoundException not found!");
        }
        rvmThrowClassNotFoundException(env, className);
    }

    TRACEF("Class '%s' loaded successfully", className);
    releaseClassLock();
    return clazz;
}

static Class* findBootClass(Env* env, const char* className) {
    Class* clazz = findClass(env, className, NULL, env->vm->options->loadBootClass);
    if (rvmExceptionOccurred(env)) return NULL;
    if (clazz != NULL) {
        if (clazz->classLoader != NULL) {
            // Not a boot class
            rvmThrowClassNotFoundException(env, className);
            return NULL;
        }
    }
    return clazz;
}


static Class* findClassByDescriptor(Env* env, const char* desc, ClassLoader* classLoader, Class* (*loaderFunc)(Env*, const char*, ClassLoader*)) {
    switch (desc[0]) {
    case 'Z':
        return prim_Z;
    case 'B':
        return prim_B;
    case 'C':
        return prim_C;
    case 'S':
        return prim_S;
    case 'I':
        return prim_I;
    case 'J':
        return prim_J;
    case 'F':
        return prim_F;
    case 'D':
        return prim_D;
    case 'V':
        return prim_V;
    case '[':
        return findClass(env, desc, classLoader, loaderFunc);
    }
    // desc[0] == 'L'
    jint length = strlen(desc);
    char* className = rvmAllocateMemory(env, length - 2 + 1);
    if (!className) return NULL;
    strncpy(className, &desc[1], length - 2);
    return findClass(env, className, classLoader, loaderFunc);
}

Class* rvmFindClassByDescriptor(Env* env, const char* desc, ClassLoader* classLoader) {
    switch (desc[0]) {
    case 'Z':
        return prim_Z;
    case 'B':
        return prim_B;
    case 'C':
        return prim_C;
    case 'S':
        return prim_S;
    case 'I':
        return prim_I;
    case 'J':
        return prim_J;
    case 'F':
        return prim_F;
    case 'D':
        return prim_D;
    case 'V':
        return prim_V;
    case '[':
        return rvmFindClassUsingLoader(env, desc, classLoader);
    }
    // desc[0] == 'L'
    jint length = strlen(desc);
    char* className = rvmAllocateMemory(env, length - 2 + 1);
    if (!className) return NULL;
    strncpy(className, &desc[1], length - 2);
    return rvmFindClassUsingLoader(env, className, classLoader);
}

char* rvmToBinaryClassName(Env* env, const char* className) {
    char* binName = rvmCopyMemoryZ(env, className);
    if (!binName) return NULL;
    jint i = 0;
    for (i = 0; binName[i] != '\0'; i++) {
        if (binName[i] == '/') binName[i] = '.';
    }
    return binName;
}

char* rvmFromBinaryClassName(Env* env, const char* binaryClassName) {
    char* className = rvmCopyMemoryZ(env, binaryClassName);
    if (!className) return NULL;
    jint i = 0;
    for (i = 0; className[i] != '\0'; i++) {
        if (className[i] == '.') className[i] = '/';
    }
    return className;
}

jboolean rvmIsSubClass(Class* superclass, Class* clazz) {
    // TODO: Array types
    while (clazz && clazz != superclass) {
        clazz = clazz->superclass;
    }
    return clazz == superclass;
}

jboolean rvmIsSamePackage(Class* c1, Class* c2) {
    if (c1 == c2) return TRUE;
    if (c1->classLoader != c2->classLoader) return FALSE;
    // TODO: Array types
    char* s1 = strrchr(c1->name, '/');
    char* s2 = strrchr(c2->name, '/');
    if (!s1 || !s2) {
        return !s1 && !s2;
    }
    int l1 = s1 - c1->name;
    int l2 = s2 - c2->name;
    if (l1 != l2) {
        return FALSE;
    }
    return strncmp(c1->name, c2->name, l1) == 0;
}

jboolean rvmIsAssignableFrom(Env* env, Class* s, Class* t) {
    // TODO: What if s or t are NULL?
    if (s == t || t == java_lang_Object) {
        return TRUE;
    }

    if (CLASS_IS_ARRAY(s)) {
        if (t == java_io_Serializable) {
            return TRUE;
        }
        if (t == java_lang_Cloneable) {
            return TRUE;
        }
        if (!CLASS_IS_ARRAY(t)) {
            return FALSE;
        }
        Class* componentType = s->componentType;
        if (CLASS_IS_PRIMITIVE(componentType)) {
            // s is a primitive array and can only be assigned to 
            // class t if s == t or t == (Object|Serializable|Cloneable). But we 
            // already know that s != t and t != (Object|Serializable|Cloneable)
            return FALSE;
        }
        return rvmIsAssignableFrom(env, componentType, t->componentType);
    }

    if (CLASS_IS_INTERFACE(t)) {
        // s or any of its parents must implement the interface t
        for (; s; s = s->superclass) {
            Interface* interface = rvmGetInterfaces(env, s);
            if (rvmExceptionCheck(env)) return FALSE;
            for (; interface != NULL; interface = interface->next) {
                if (rvmIsAssignableFrom(env, interface->interface, t)) {
                    return TRUE;
                }
            }
        }
        return FALSE;
    }

    while (s && s != t) {
        s = s->superclass;
    }
    return s ? TRUE : FALSE;
}

jboolean rvmIsInstanceOf(Env* env, Object* obj, Class* clazz) {
    if (!obj) return FALSE;
    return rvmIsAssignableFrom(env, obj->clazz, clazz);
}

static jboolean fixClassPointer(Env* env, Class* c, void* data) {
    c->object.clazz = java_lang_Class;
    return TRUE;
}

jboolean rvmInitClasses(Env* env) {

    if (hythread_monitor_init_with_name(&classLock, 0, NULL) < 0) {
        return FALSE;
    }

    // Cache important classes in java.lang.
    java_lang_Object = findBootClass(env, "java/lang/Object");
    if (!java_lang_Object) return FALSE;
    java_lang_Class = findBootClass(env, "java/lang/Class");
    if (!java_lang_Class) return FALSE;

    // Fix object.clazz pointers for the classes loaded so far
    rvmIterateLoadedClasses(env, fixClassPointer, NULL);

    java_lang_ClassLoader = findBootClass(env, "java/lang/ClassLoader");
    if (!java_lang_ClassLoader) return FALSE;
    java_lang_String = findBootClass(env, "java/lang/String");
    if (!java_lang_String) return FALSE;
    java_lang_Boolean = findBootClass(env, "java/lang/Boolean");
    if (!java_lang_Boolean) return FALSE;
    java_lang_Byte = findBootClass(env, "java/lang/Byte");
    if (!java_lang_Byte) return FALSE;
    java_lang_Character = findBootClass(env, "java/lang/Character");
    if (!java_lang_Character) return FALSE;
    java_lang_Short = findBootClass(env, "java/lang/Short");
    if (!java_lang_Short) return FALSE;
    java_lang_Integer = findBootClass(env, "java/lang/Integer");
    if (!java_lang_Integer) return FALSE;
    java_lang_Long = findBootClass(env, "java/lang/Long");
    if (!java_lang_Long) return FALSE;
    java_lang_Float = findBootClass(env, "java/lang/Float");
    if (!java_lang_Float) return FALSE;
    java_lang_Double = findBootClass(env, "java/lang/Double");
    if (!java_lang_Double) return FALSE;
    java_lang_Enum = findBootClass(env, "java/lang/Enum");
    if (!java_lang_Enum) return FALSE;
    java_lang_Cloneable = findBootClass(env, "java/lang/Cloneable");
    if (!java_lang_Cloneable) return FALSE;
    java_lang_Thread = findBootClass(env, "java/lang/Thread");
    if (!java_lang_Thread) return FALSE;
    java_lang_ThreadGroup = findBootClass(env, "java/lang/ThreadGroup");
    if (!java_lang_ThreadGroup) return FALSE;
    java_io_Serializable = findBootClass(env, "java/io/Serializable");
    if (!java_io_Serializable) return FALSE;
    java_lang_Runtime = findBootClass(env, "java/lang/Runtime");
    if (!java_lang_Runtime) return FALSE;

    java_lang_ClassNotFoundException = findBootClass(env, "java/lang/ClassNotFoundException");
    if (!java_lang_ClassNotFoundException) return FALSE;
    java_lang_NoClassDefFoundError = findBootClass(env, "java/lang/NoClassDefFoundError");
    if (!java_lang_NoClassDefFoundError) return FALSE;
    java_lang_Error = findBootClass(env, "java/lang/Error");
    if (!java_lang_Error) return FALSE;
    java_lang_OutOfMemoryError = findBootClass(env, "java/lang/OutOfMemoryError");
    if (!java_lang_OutOfMemoryError) return FALSE;
    java_lang_IllegalAccessError = findBootClass(env, "java/lang/IllegalAccessError");
    if (!java_lang_IllegalAccessError) return FALSE;
    java_lang_NoSuchFieldError = findBootClass(env, "java/lang/NoSuchFieldError");
    if (!java_lang_NoSuchFieldError) return FALSE;
    java_lang_NoSuchMethodError = findBootClass(env, "java/lang/NoSuchMethodError");
    if (!java_lang_NoSuchMethodError) return FALSE;
    java_lang_IncompatibleClassChangeError = findBootClass(env, "java/lang/IncompatibleClassChangeError");
    if (!java_lang_IncompatibleClassChangeError) return FALSE;
    java_lang_AbstractMethodError = findBootClass(env, "java/lang/AbstractMethodError");
    if (!java_lang_AbstractMethodError) return FALSE;
    java_lang_UnsatisfiedLinkError = findBootClass(env, "java/lang/UnsatisfiedLinkError");
    if (!java_lang_UnsatisfiedLinkError) return FALSE;
    java_lang_ExceptionInInitializerError = findBootClass(env, "java/lang/ExceptionInInitializerError");
    if (!java_lang_ExceptionInInitializerError) return FALSE;
    java_lang_VerifyError = findBootClass(env, "java/lang/VerifyError");
    if (!java_lang_VerifyError) return FALSE;
    java_lang_LinkageError = findBootClass(env, "java/lang/LinkageError");
    if (!java_lang_LinkageError) return FALSE;
    java_lang_InstantiationError = findBootClass(env, "java/lang/InstantiationError");
    if (!java_lang_InstantiationError) return FALSE;

    java_lang_Throwable = findBootClass(env, "java/lang/Throwable");
    if (!java_lang_Throwable) return FALSE;
    java_lang_RuntimeException = findBootClass(env, "java/lang/RuntimeException");
    if (!java_lang_RuntimeException) return FALSE;
    java_lang_ClassCastException = findBootClass(env, "java/lang/ClassCastException");
    if (!java_lang_ClassCastException) return FALSE;
    java_lang_NullPointerException = findBootClass(env, "java/lang/NullPointerException");
    if (!java_lang_NullPointerException) return FALSE;
    java_lang_ArrayIndexOutOfBoundsException = findBootClass(env, "java/lang/ArrayIndexOutOfBoundsException");
    if (!java_lang_ArrayIndexOutOfBoundsException) return FALSE;
    java_lang_ArrayStoreException = findBootClass(env, "java/lang/ArrayStoreException");
    if (!java_lang_ArrayStoreException) return FALSE;
    java_lang_NegativeArraySizeException = findBootClass(env, "java/lang/NegativeArraySizeException");
    if (!java_lang_NegativeArraySizeException) return FALSE;
    java_lang_IllegalArgumentException = findBootClass(env, "java/lang/IllegalArgumentException");
    if (!java_lang_IllegalArgumentException) return FALSE;
    java_lang_ArithmeticException = findBootClass(env, "java/lang/ArithmeticException");
    if (!java_lang_ArithmeticException) return FALSE;
    java_lang_UnsupportedOperationException = findBootClass(env, "java/lang/UnsupportedOperationException");
    if (!java_lang_UnsupportedOperationException) return FALSE;
    java_lang_IllegalMonitorStateException = findBootClass(env, "java/lang/IllegalMonitorStateException");
    if (!java_lang_IllegalMonitorStateException) return FALSE;
    java_lang_InstantiationException = findBootClass(env, "java/lang/InstantiationException");
    if (!java_lang_InstantiationException) return FALSE;
    java_lang_InterruptedException = findBootClass(env, "java/lang/InterruptedException");
    if (!java_lang_InterruptedException) return FALSE;

    prim_Z = createPrimitiveClass(env, "Z");
    if (!prim_Z) return FALSE;
    prim_B = createPrimitiveClass(env, "B");
    if (!prim_B) return FALSE;
    prim_C = createPrimitiveClass(env, "C");
    if (!prim_C) return FALSE;
    prim_S = createPrimitiveClass(env, "S");
    if (!prim_S) return FALSE;
    prim_I = createPrimitiveClass(env, "I");
    if (!prim_I) return FALSE;
    prim_J = createPrimitiveClass(env, "J");
    if (!prim_J) return FALSE;
    prim_F = createPrimitiveClass(env, "F");
    if (!prim_F) return FALSE;
    prim_D = createPrimitiveClass(env, "D");
    if (!prim_D) return FALSE;
    prim_V = createPrimitiveClass(env, "V");
    if (!prim_V) return FALSE;

    array_Z = findBootClass(env, "[Z");
    if (!array_Z) return FALSE;
    array_B = findBootClass(env, "[B");
    if (!array_B) return FALSE;
    array_C = findBootClass(env, "[C");
    if (!array_C) return FALSE;
    array_S = findBootClass(env, "[S");
    if (!array_S) return FALSE;
    array_I = findBootClass(env, "[I");
    if (!array_I) return FALSE;
    array_J = findBootClass(env, "[J");
    if (!array_J) return FALSE;
    array_F = findBootClass(env, "[F");
    if (!array_F) return FALSE;
    array_D = findBootClass(env, "[D");
    if (!array_D) return FALSE;

    return TRUE;
}

jboolean rvmInitPrimitiveWrapperClasses(Env* env) {
    Class* c = NULL;
    ClassField* f = NULL;

    f = rvmGetClassField(env, java_lang_Boolean, "TRUE", "Ljava/lang/Boolean;");
    if (!f) return FALSE;
    java_lang_Boolean_TRUE = (Boolean*) rvmGetObjectClassFieldValue(env, java_lang_Boolean, f);
    if (!java_lang_Boolean_TRUE) return FALSE;

    f = rvmGetClassField(env, java_lang_Boolean, "FALSE", "Ljava/lang/Boolean;");
    if (!f) return FALSE;
    java_lang_Boolean_FALSE = (Boolean*) rvmGetObjectClassFieldValue(env, java_lang_Boolean, f);
    if (!java_lang_Boolean_FALSE) return FALSE;

    java_lang_Byte_valueOf = rvmGetClassMethod(env, java_lang_Byte, "valueOf", "(B)Ljava/lang/Byte;");
    if (!java_lang_Byte_valueOf) return FALSE;
    f = rvmGetClassField(env, java_lang_Byte, "VALUES", "[Ljava/lang/Byte;");
    if (!f) return FALSE;
    bytesCache = (ObjectArray*) rvmGetObjectClassFieldValue(env, java_lang_Byte, f);
    if (!bytesCache) return FALSE;

    java_lang_Short_valueOf = rvmGetClassMethod(env, java_lang_Short, "valueOf", "(S)Ljava/lang/Short;");
    if (!java_lang_Short_valueOf) return FALSE;
    f = rvmGetClassField(env, java_lang_Short, "SMALL_VALUES", "[Ljava/lang/Short;");
    if (!f) return FALSE;
    shortsCache = (ObjectArray*) rvmGetObjectClassFieldValue(env, c, f);
    if (!shortsCache) return FALSE;

    java_lang_Character_valueOf = rvmGetClassMethod(env, java_lang_Character, "valueOf", "(C)Ljava/lang/Character;");
    if (!java_lang_Character_valueOf) return FALSE;
    f = rvmGetClassField(env, java_lang_Character, "SMALL_VALUES", "[Ljava/lang/Character;");
    if (!f) return FALSE;
    charactersCache = (ObjectArray*) rvmGetObjectClassFieldValue(env, c, f);
    if (!charactersCache) return FALSE;

    java_lang_Integer_valueOf = rvmGetClassMethod(env, java_lang_Integer, "valueOf", "(I)Ljava/lang/Integer;");
    if (!java_lang_Integer_valueOf) return FALSE;
    f = rvmGetClassField(env, java_lang_Integer, "SMALL_VALUES", "[Ljava/lang/Integer;");
    if (!f) return FALSE;
    integersCache = (ObjectArray*) rvmGetObjectClassFieldValue(env, c, f);
    if (!integersCache) return FALSE;

    java_lang_Long_valueOf = rvmGetClassMethod(env, java_lang_Long, "valueOf", "(J)Ljava/lang/Long;");
    if (!java_lang_Long_valueOf) return FALSE;
    f = rvmGetClassField(env, java_lang_Long, "SMALL_VALUES", "[Ljava/lang/Long;");
    if (!f) return FALSE;
    longsCache = (ObjectArray*) rvmGetObjectClassFieldValue(env, c, f);
    if (!longsCache) return FALSE;

    java_lang_Float_valueOf = rvmGetClassMethod(env, java_lang_Float, "valueOf", "(F)Ljava/lang/Float;");
    if (!java_lang_Float_valueOf) return FALSE;

    java_lang_Double_valueOf = rvmGetClassMethod(env, java_lang_Double, "valueOf", "(D)Ljava/lang/Double;");
    if (!java_lang_Double_valueOf) return FALSE;

    return TRUE;
}

Class* rvmFindClass(Env* env, const char* className) {
    Method* method = rvmGetCallingMethod(env);
    if (rvmExceptionOccurred(env)) return NULL;
    ClassLoader* classLoader = method ? method->clazz->classLoader : NULL;
    return rvmFindClassUsingLoader(env, className, classLoader);
}

Class* rvmFindClassInClasspathForLoader(Env* env, const char* className, ClassLoader* classLoader) {
    if (!classLoader || classLoader->parent == NULL) {
        // This is the bootstrap classloader
        return findBootClass(env, className);
    }
    if (classLoader->parent->parent == NULL && classLoader->object.clazz->classLoader == NULL) {
        // This is the system classloader
        Class* clazz = findClass(env, className, classLoader, env->vm->options->loadUserClass);
        if (rvmExceptionOccurred(env)) return NULL;
        return clazz;
    }
    rvmThrowClassNotFoundException(env, className);
    return NULL;
}

Class* rvmFindClassUsingLoader(Env* env, const char* className, ClassLoader* classLoader) {
    if (!classLoader || classLoader->parent == NULL) {
        // This is the bootstrap classloader. No need to call ClassLoader.loadClass()
        return findBootClass(env, className);
    }
    char* binaryClassName = rvmToBinaryClassName(env, className);
    if (!binaryClassName) return NULL;
    Object* binaryClassNameString = rvmNewInternedStringUTF(env, binaryClassName, -1);
    if (!binaryClassNameString) return NULL;
    Method* loadClassMethod = rvmGetInstanceMethod(env, java_lang_ClassLoader, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
    if (!loadClassMethod) return NULL;
    Object* clazz = rvmCallObjectInstanceMethod(env, (Object*) classLoader, loadClassMethod, binaryClassNameString);
    if (rvmExceptionOccurred(env)) return NULL;
    return (Class*) clazz;
}

Class* rvmFindLoadedClass(Env* env, const char* className, ClassLoader* classLoader) {
    Class* clazz = getLoadedClass(env, className);
    if (rvmExceptionOccurred(env)) return NULL;
    return clazz;
}

ClassLoader* rvmGetSystemClassLoader(Env* env) {
    Class* holder = rvmFindClass(env, "java/lang/ClassLoader$SystemClassLoader");
    if (!holder) return NULL;
    ClassField* field = rvmGetClassField(env, holder, "loader", "Ljava/lang/ClassLoader;");
    if (!field) return NULL;
    return (ClassLoader*) rvmGetObjectClassFieldValue(env, holder, field);
}

Class* rvmAllocateClass(Env* env, const char* className, Class* superclass, ClassLoader* classLoader, jint flags, 
        jint classDataSize, jint instanceDataSize, jint instanceDataOffset, void* attributes, void* initializer) {

    if (superclass && CLASS_IS_INTERFACE(superclass)) {
        // TODO: Message should look like ?
        rvmThrowIncompatibleClassChangeError(env, "");
        return NULL;
    }

    Class* clazz = rvmAllocateMemory(env, classDataSize);
    if (!clazz) return NULL;

    /*
     * NOTE: All classes we load before we have cached java.lang.Class will have NULL here so it is 
     * important that we cache java.lang.Class as soon as possible. However, we have to cache
     * java.lang.Object first since it is the superclass of java.lang.Class. This means that
     * the java_lang_Object global variable will actually have NULL as clazz until we fix this in
     * rvmInitClasses().
     */
    clazz->object.clazz = java_lang_Class;
    clazz->name = className;
    clazz->superclass = superclass;
    clazz->classLoader = classLoader;
    clazz->flags = flags;
    clazz->classDataSize = classDataSize;
    clazz->instanceDataSize = instanceDataSize;
    clazz->instanceDataOffset = instanceDataOffset;
    clazz->_interfaces = &INTERFACES_NOT_LOADED;
    clazz->_fields = &FIELDS_NOT_LOADED;
    clazz->_methods = &METHODS_NOT_LOADED;
    clazz->attributes = attributes;
    clazz->initializer = initializer;

    return clazz;
}

jboolean rvmAddInterface(Env* env, Class* clazz, Class* interf) {
    if (!CLASS_IS_INTERFACE(interf)) {
        // TODO: Message should look like ?
        rvmThrowIncompatibleClassChangeError(env, "");
        return FALSE;
    }
    Interface* interface = rvmAllocateMemory(env, sizeof(Interface));
    if (!interface) return FALSE;
    interface->interface = interf;
    if (clazz->_interfaces == &INTERFACES_NOT_LOADED) {
        clazz->_interfaces = NULL;
    }
    LL_APPEND(clazz->_interfaces, interface);
    return TRUE;
}

Method* rvmAddMethod(Env* env, Class* clazz, const char* name, const char* desc, jint access, jint size, void* impl, void* synchronizedImpl, void* attributes) {
    Method* method = rvmAllocateMemory(env, IS_NATIVE(access) ? sizeof(NativeMethod) : sizeof(Method));
    if (!method) return NULL;
    method->clazz = clazz;
    method->name = name;
    method->desc = desc;
    method->access = access;
    method->size = size;
    method->impl = impl;
    method->synchronizedImpl = synchronizedImpl;
    method->attributes = attributes;

    if (clazz->_methods == &METHODS_NOT_LOADED) {
        clazz->_methods = NULL;
    }

    method->next = clazz->_methods;
    clazz->_methods = method;

    return method;
}

ProxyMethod* addProxyMethod(Env* env, Class* clazz, Method* proxiedMethod, jint access, void* impl) {
    ProxyMethod* method = rvmAllocateMemory(env, sizeof(ProxyMethod));
    if (!method) return NULL;
    method->method.clazz = clazz;
    method->method.name = proxiedMethod->name;
    method->method.desc = proxiedMethod->desc;
    method->method.access = access | METHOD_TYPE_PROXY;
    method->method.impl = impl;
    method->method.synchronizedImpl = NULL;
    method->proxiedMethod = proxiedMethod;

    if (clazz->_methods == &METHODS_NOT_LOADED) {
        clazz->_methods = NULL;
    }

    method->method.next = clazz->_methods;
    clazz->_methods = (Method*) method;

    return method;
}

BridgeMethod* rvmAddBridgeMethod(Env* env, Class* clazz, const char* name, const char* desc, jint access, jint size, void* impl, 
        void* synchronizedImpl, void** targetFnPtr, void* attributes) {
    
    BridgeMethod* method = rvmAllocateMemory(env, sizeof(BridgeMethod));
    if (!method) return NULL;
    method->method.clazz = clazz;
    method->method.name = name;
    method->method.desc = desc;
    method->method.access = access | METHOD_TYPE_BRIDGE;
    method->method.size = size;
    method->method.impl = impl;
    method->method.synchronizedImpl = synchronizedImpl;
    method->method.attributes = attributes;
    method->targetFnPtr = targetFnPtr;

    if (clazz->_methods == &METHODS_NOT_LOADED) {
        clazz->_methods = NULL;
    }

    method->method.next = clazz->_methods;
    clazz->_methods = (Method*) method;

    return method;
}

CallbackMethod* rvmAddCallbackMethod(Env* env, Class* clazz, const char* name, const char* desc, jint access, jint size, void* impl, 
        void* synchronizedImpl, void* callbackImpl, void* attributes) {
    
    CallbackMethod* method = rvmAllocateMemory(env, sizeof(CallbackMethod));
    if (!method) return NULL;
    method->method.clazz = clazz;
    method->method.name = name;
    method->method.desc = desc;
    method->method.access = access | METHOD_TYPE_CALLBACK;
    method->method.size = size;
    method->method.impl = impl;
    method->method.synchronizedImpl = synchronizedImpl;
    method->method.attributes = attributes;
    method->callbackImpl = callbackImpl;

    if (clazz->_methods == &METHODS_NOT_LOADED) {
        clazz->_methods = NULL;
    }

    method->method.next = clazz->_methods;
    clazz->_methods = (Method*) method;

    return method;
}

Field* rvmAddField(Env* env, Class* clazz, const char* name, const char* desc, jint access, jint offset, void* attributes) {
    Field* field = rvmAllocateMemory(env, IS_STATIC(access) ? sizeof(ClassField) : sizeof(InstanceField));
    if (!field) return NULL;
    field->clazz = clazz;
    field->name = name;
    field->desc = desc;
    field->access = access;
    field->attributes = attributes;

    if (clazz->_fields == &FIELDS_NOT_LOADED) {
        clazz->_fields = NULL;
    }

    field->next = clazz->_fields;
    clazz->_fields = field;

    if (access & ACC_STATIC) {
        ((ClassField*) field)->address = ((jbyte*) clazz) + offset;
    } else {
        ((InstanceField*) field)->offset = offset;
    }
    return field;
}

Interface* rvmGetInterfaces(Env* env, Class* clazz) {
    if (clazz->_interfaces != &INTERFACES_NOT_LOADED) return clazz->_interfaces;

    // TODO: Double checked locking
    obtainClassLock();
    if (clazz->_interfaces == &INTERFACES_NOT_LOADED) {
        env->vm->options->loadInterfaces(env, clazz);
        if (clazz->_interfaces == &INTERFACES_NOT_LOADED) {
            // The class has no interfaces
            clazz->_interfaces = NULL;
        }
    }
    if (rvmExceptionCheck(env)) clazz->_interfaces = &INTERFACES_NOT_LOADED;
    releaseClassLock();
    if (rvmExceptionCheck(env)) return NULL;
    return clazz->_interfaces;
}

Field* rvmGetFields(Env* env, Class* clazz) {
    if (clazz->_fields != &FIELDS_NOT_LOADED) return clazz->_fields;

    // TODO: Double checked locking
    obtainClassLock();
    if (clazz->_fields == &FIELDS_NOT_LOADED) {
        env->vm->options->loadFields(env, clazz);
        if (clazz->_fields == &FIELDS_NOT_LOADED) {
            // The class has no fields
            clazz->_fields = NULL;
        }
    }
    if (rvmExceptionCheck(env)) clazz->_fields = &FIELDS_NOT_LOADED;
    releaseClassLock();
    if (rvmExceptionCheck(env)) return NULL;
    return clazz->_fields;
}

Method* rvmGetMethods(Env* env, Class* clazz) {
    if (clazz->_methods != &METHODS_NOT_LOADED) return clazz->_methods;

    // TODO: Double checked locking
    obtainClassLock();
    if (clazz->_methods == &METHODS_NOT_LOADED) {
        env->vm->options->loadMethods(env, clazz);
        if (clazz->_methods == &METHODS_NOT_LOADED) {
            // The class has no methods
            clazz->_methods = NULL;
        }
    }
    if (rvmExceptionCheck(env)) clazz->_methods = &METHODS_NOT_LOADED;
    releaseClassLock();
    if (rvmExceptionCheck(env)) return NULL;
    return clazz->_methods;
}

jboolean rvmRegisterClass(Env* env, Class* clazz) {
    // TODO: Check that the superclass and all interfaces are accessible to the new class
    // TODO: Verify the class hierarchy (class doesn't override final methods, changes public -> private, etc)

    obtainClassLock();
    if (!addLoadedClass(env, clazz)) {
        releaseClassLock();
        return FALSE;
    }

    clazz->flags = (clazz->flags & (~CLASS_STATE_MASK)) | CLASS_STATE_LOADED;

    releaseClassLock();
    return TRUE;
}

void rvmInitialize(Env* env, Class* clazz) {
    obtainClassLock();
    // TODO: Throw java.lang.NoClassDefFoundError if state == CLASS_ERROR?
    if (CLASS_IS_STATE_ERROR(clazz)) {
        // TODO: Add the class' binary name in the message
        rvmThrowNew(env, java_lang_NoClassDefFoundError, "Could not initialize class ??");
        releaseClassLock();
        return;
    }
    if (!CLASS_IS_STATE_INITIALIZED(clazz) && !CLASS_IS_STATE_INITIALIZING(clazz)) {
        jint oldState = clazz->flags & CLASS_STATE_MASK;
        clazz->flags = (clazz->flags & (~CLASS_STATE_MASK)) | CLASS_STATE_INITIALIZING;
        if (clazz->superclass) {
            rvmInitialize(env, clazz->superclass);
            if (rvmExceptionOccurred(env)) {
                clazz->flags = (clazz->flags & (~CLASS_STATE_MASK)) | oldState;
                releaseClassLock();
                return;
            }
        }

        TRACEF("Initializing class %s", clazz->name);
        void* initializer = clazz->initializer;
        if (!initializer) {
            if (!CLASS_IS_ARRAY(clazz) && !CLASS_IS_PROXY(clazz) && !CLASS_IS_PRIMITIVE(clazz)) {
                env->vm->options->classInitialized(env, clazz);
            }
            clazz->flags = (clazz->flags & (~CLASS_STATE_MASK)) | CLASS_STATE_INITIALIZED;
            releaseClassLock();
            return;
        }

        CallInfo* callInfo = call0AllocateCallInfo(env, initializer, 1, 0, 0, 0, 0);
        call0AddPtr(callInfo, env);
        void (*f)(CallInfo*) = (void (*)(CallInfo*)) _call0;
        rvmPushGatewayFrame(env);
        f(callInfo);
        rvmPopGatewayFrame(env);

        Object* exception = rvmExceptionClear(env);
        if (exception) {
            clazz->flags = (clazz->flags & (~CLASS_STATE_MASK)) | CLASS_STATE_ERROR;
            if (!rvmIsInstanceOf(env, exception, java_lang_Error)) {
                // If exception isn't an instance of java.lang.Error 
                // we must wrap it in a java.lang.ExceptionInInitializerError
                Method* constructor = rvmGetInstanceMethod(env, java_lang_ExceptionInInitializerError, "<init>", "(Ljava/lang/Throwable;)V");
                if (!constructor) return;
                Object* wrappedException = rvmNewObject(env, java_lang_ExceptionInInitializerError, constructor, exception);
                if (!wrappedException) return;
                exception = wrappedException;
            }
            rvmThrow(env, exception);
            releaseClassLock();
            return;
        }
        if (!CLASS_IS_ARRAY(clazz) && !CLASS_IS_PROXY(clazz) && !CLASS_IS_PRIMITIVE(clazz)) {
            env->vm->options->classInitialized(env, clazz);
        }
        clazz->flags = (clazz->flags & (~CLASS_STATE_MASK)) | CLASS_STATE_INITIALIZED;
    }
    releaseClassLock();
}

Object* rvmAllocateObject(Env* env, Class* clazz) {
    if (CLASS_IS_ABSTRACT(clazz) || CLASS_IS_INTERFACE(clazz)) {
        // TODO: Message
        rvmThrowNew(env, java_lang_InstantiationException, "");
        return NULL;
    }
    rvmInitialize(env, clazz);
    if (rvmExceptionOccurred(env)) return NULL;
    Object* obj = rvmAllocateMemory(env, clazz->instanceDataSize);
    if (!obj) return NULL;
    obj->clazz = clazz;
    return obj;
}

Object* rvmNewObject(Env* env, Class* clazz, Method* method, ...) {
    va_list args;
    va_start(args, method);
    return rvmNewObjectV(env, clazz, method, args);
}

Object* rvmNewObjectA(Env* env, Class* clazz, Method* method, jvalue *args) {
    Object* obj = rvmAllocateObject(env, clazz);
    if (!obj) return NULL;
    rvmCallNonvirtualVoidInstanceMethodA(env, obj, method, args);
    if (rvmExceptionOccurred(env)) return NULL;
    return obj;
}

Object* rvmNewObjectV(Env* env, Class* clazz, Method* method, va_list args) {
    Object* obj = rvmAllocateObject(env, clazz);
    if (!obj) return NULL;
    rvmCallNonvirtualVoidInstanceMethodV(env, obj, method, args);
    if (rvmExceptionOccurred(env)) return NULL;
    return obj;
}

Boolean* rvmNewBoolean(Env* env, jboolean value) {
    return value ? java_lang_Boolean_TRUE : java_lang_Boolean_FALSE;
}

Byte* rvmNewByte(Env* env, jbyte value) {
    jint index = value + 128;
    if (index >= 0 && index < bytesCache->length && bytesCache->values[index] != NULL) {
        return (Byte*) bytesCache->values[index];
    }
    jvalue args[1];
    args[0].b = value;
    return (Byte*) rvmCallObjectClassMethodA(env, java_lang_Byte, java_lang_Byte_valueOf, args);
}

Short* rvmNewShort(Env* env, jshort value) {
    jint index = value + 128;
    if (index >= 0 && index < shortsCache->length && shortsCache->values[index] != NULL) {
        return (Short*) shortsCache->values[index];
    }
    jvalue args[1];
    args[0].s = value;
    return (Short*) rvmCallObjectClassMethodA(env, java_lang_Short, java_lang_Short_valueOf, args);
}

Character* rvmNewCharacter(Env* env, jchar value) {
    jint index = value;
    if (index >= 0 && index < charactersCache->length && charactersCache->values[index] != NULL) {
        return (Character*) charactersCache->values[index];
    }
    jvalue args[1];
    args[0].c = value;
    return (Character*) rvmCallObjectClassMethodA(env, java_lang_Character, java_lang_Character_valueOf, args);
}

Integer* rvmNewInteger(Env* env, jint value) {
    jint index = value + 128;
    if (index >= 0 && index < integersCache->length && integersCache->values[index] != NULL) {
        return (Integer*) integersCache->values[index];
    }
    jvalue args[1];
    args[0].i = value;
    return (Integer*) rvmCallObjectClassMethodA(env, java_lang_Integer, java_lang_Integer_valueOf, args);
}

Long* rvmNewLong(Env* env, jlong value) {
    jint index = value + 128;
    if (index >= 0 && index < longsCache->length && longsCache->values[index] != NULL) {
        return (Long*) longsCache->values[index];
    }
    jvalue args[1];
    args[0].j = value;
    return (Long*) rvmCallObjectClassMethodA(env, java_lang_Long, java_lang_Long_valueOf, args);
}

Float* rvmNewFloat(Env* env, jfloat value) {
    jvalue args[1];
    args[0].f = value;
    return (Float*) rvmCallObjectClassMethodA(env, java_lang_Float, java_lang_Float_valueOf, args);
}

Double* rvmNewDouble(Env* env, jdouble value) {
    jvalue args[1];
    args[0].d = value;
    return (Double*) rvmCallObjectClassMethodA(env, java_lang_Double, java_lang_Double_valueOf, args);
}

Object* rvmWrapPrimitive(Env* env, Class* type, jvalue* value) {
    if (CLASS_IS_PRIMITIVE(type)) {
        switch (type->name[0]) {
        case 'Z':
            return (Object*) rvmNewBoolean(env, value->z);
        case 'B':
            return (Object*) rvmNewByte(env, value->b);
        case 'S':
            return (Object*) rvmNewShort(env, value->s);
        case 'C':
            return (Object*) rvmNewCharacter(env, value->c);
        case 'I':
            return (Object*) rvmNewInteger(env, value->i);
        case 'J':
            return (Object*) rvmNewLong(env, value->j);
        case 'F':
            return (Object*) rvmNewFloat(env, value->f);
        case 'D':
            return (Object*) rvmNewDouble(env, value->d);
        }
    }
    return (Object*) value->l;
}

Object* rvmCloneObject(Env* env, Object* obj) {
    if (CLASS_IS_ARRAY(obj->clazz)) {
        return (Object*) rvmCloneArray(env, (Array*) obj);
    }
    jint size = obj->clazz->instanceDataSize;
    Object* copy = rvmAllocateMemory(env, size);
    if (!copy) return NULL;
    memcpy(copy, obj, size);
    copy->monitor = NULL;
    return copy;
}

void rvmIterateLoadedClasses(Env* env, jboolean (*f)(Env*, Class*, void*), void* data) {
    LoadedClassEntry* entry;
    for (entry = loadedClasses; entry != NULL; entry = entry->hh.next) {
        if (!f(env, entry->clazz, data)) return;
    }
}

static jboolean dumpClassesIterator(Env* env, Class* clazz, void* d) {
    fprintf(stderr, "%p: %s\n", clazz, clazz->name);
    return TRUE;
}

void rvmDumpLoadedClasses(Env* env) {
    rvmIterateLoadedClasses(env, dumpClassesIterator, NULL);
}

