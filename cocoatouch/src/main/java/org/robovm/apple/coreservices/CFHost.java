/*
 * Copyright (C) 2014 Trillian Mobile AB
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
package org.robovm.apple.coreservices;

/*<imports>*/
import java.io.*;
import java.nio.*;
import java.util.*;
import org.robovm.objc.*;
import org.robovm.objc.annotation.*;
import org.robovm.objc.block.*;
import org.robovm.rt.*;
import org.robovm.rt.bro.*;
import org.robovm.rt.bro.annotation.*;
import org.robovm.rt.bro.ptr.*;
import org.robovm.apple.foundation.*;
import org.robovm.apple.corefoundation.*;
/*</imports>*/
import org.robovm.apple.coremedia.CMBufferQueueCallbacks;
import org.robovm.apple.coremedia.CMBufferQueueTriggerToken;
import org.robovm.apple.coremedia.CMTime;

/*<javadoc>*/
/*</javadoc>*/
/*<annotations>*/@Library("CFNetwork")/*</annotations>*/
/*<visibility>*/public/*</visibility>*/ class /*<name>*/CFHost/*</name>*/ 
    extends /*<extends>*/CFType/*</extends>*/ 
    /*<implements>*//*</implements>*/ {

    /*<ptr>*/public static class CFHostPtr extends Ptr<CFHost, CFHostPtr> {}/*</ptr>*/
    
    public interface ClientCallback {
        void invoke(CFHost host, CFHostInfoType infoType, CFStreamError error);
    }
    
    private static java.util.concurrent.atomic.AtomicLong refconId = new java.util.concurrent.atomic.AtomicLong();
    private long localRefconId;
    private static Map<Long, ClientCallback> callbacks = new HashMap<Long, ClientCallback>();
    private static final java.lang.reflect.Method cbInvoke;
    
    static {
        try {
            cbInvoke = CFHost.class.getDeclaredMethod("cbInvoke", CFHost.class, CFHostInfoType.class, CFStreamError.CFStreamErrorPtr.class, long.class);
        } catch (Throwable e) {
            throw new Error(e);
        }
    }
    /*<bind>*/static { Bro.bind(CFHost.class); }/*</bind>*/
    /*<constants>*//*</constants>*/
    /*<constructors>*/
    protected CFHost() {}
    /*</constructors>*/
    /*<properties>*//*</properties>*/
    /*<members>*//*</members>*/
    @Callback
    private static void cbInvoke(CFHost host, CFHostInfoType infoType, CFStreamError.CFStreamErrorPtr error, @Pointer long refcon) {
        ClientCallback callback = null;
        synchronized (callbacks) {
            callback = callbacks.get(refcon);
        }
        CFStreamError err = null;
        if (error != null) err = error.get();
        callback.invoke(host, infoType, err);
    }
    /**
     * @since Available in iOS 2.0 and later.
     */
    public static CFHost create(String hostname) {
       long refconId = CFHost.refconId.getAndIncrement();
       CFHost host = create(null, hostname);
       if (host != null) host.localRefconId = refconId;
       return host;
    }
    /**
     * @since Available in iOS 2.0 and later.
     */
    public static CFHost create(NSData addr) {
        long refconId = CFHost.refconId.getAndIncrement();
        CFHost host = create(null, addr);
        if (host != null) host.localRefconId = refconId;
        return host;
    }
    /**
     * @since Available in iOS 2.0 and later.
     */
    public static CFHost createCopy(CFHost host) {
        CFHost h = createCopy(null, host);
        if (host != null) h.localRefconId = host.localRefconId;
        return h;
    }
    /**
     * @since Available in iOS 2.0 and later.
     */
    public boolean startInfoResolution(CFHostInfoType info) {
        return startInfoResolution(info, null);
    }
    /**
     * @since Available in iOS 2.0 and later.
     */
    public NSArray<NSData> getAddressing() {
        return getAddressing(null);
    }
    /**
     * @since Available in iOS 2.0 and later.
     */
    public List<String> getNames() {
        return getNames(null);
    }
    /**
     * @since Available in iOS 2.0 and later.
     */
    public NSData getReachability() {
        return getReachability(null);
    }
    /**
     * @since Available in iOS 2.0 and later.
     */
    public void setCallback(ClientCallback callback) {
        CFHostClientContext context = new CFHostClientContext();
        context.info(localRefconId);
        synchronized (callbacks) {
            callbacks.put(localRefconId, callback);
        }
        setCallback(new FunctionPtr(cbInvoke), context);
    }
    /**
     * @since Available in iOS 2.0 and later.
     */
    public CFReadStream createSocketReadStream(int port) {
        CFReadStream.CFReadStreamPtr ptr = new CFReadStream.CFReadStreamPtr();
        createSocketStreamPair(null, this, port, ptr, null);
        return ptr.get();
    }
    /**
     * @since Available in iOS 2.0 and later.
     */
    public CFWriteStream createSocketWriteStream(int port) {
        CFWriteStream.CFWriteStreamPtr ptr = new CFWriteStream.CFWriteStreamPtr();
        createSocketStreamPair(null, this, port, null, ptr);
        return ptr.get();
    }
    /*<methods>*/
    /**
     * @since Available in iOS 2.0 and later.
     */
    @Bridge(symbol="CFHostGetTypeID", optional=true)
    public static native @MachineSizedUInt long getClassTypeID();
    /**
     * @since Available in iOS 2.0 and later.
     */
    @Bridge(symbol="CFHostCreateWithName", optional=true)
    protected static native CFHost create(CFAllocator allocator, String hostname);
    /**
     * @since Available in iOS 2.0 and later.
     */
    @Bridge(symbol="CFHostCreateWithAddress", optional=true)
    protected static native CFHost create(CFAllocator allocator, NSData addr);
    /**
     * @since Available in iOS 2.0 and later.
     */
    @Bridge(symbol="CFHostCreateCopy", optional=true)
    protected static native CFHost createCopy(CFAllocator alloc, CFHost host);
    /**
     * @since Available in iOS 2.0 and later.
     */
    @Bridge(symbol="CFHostStartInfoResolution", optional=true)
    protected native boolean startInfoResolution(CFHostInfoType info, CFStreamError.CFStreamErrorPtr error);
    /**
     * @since Available in iOS 2.0 and later.
     */
    @Bridge(symbol="CFHostGetAddressing", optional=true)
    protected native NSArray<NSData> getAddressing(BytePtr hasBeenResolved);
    /**
     * @since Available in iOS 2.0 and later.
     */
    @Bridge(symbol="CFHostGetNames", optional=true)
    protected native @org.robovm.rt.bro.annotation.Marshaler(CFArray.AsStringListMarshaler.class) List<String> getNames(BytePtr hasBeenResolved);
    /**
     * @since Available in iOS 2.0 and later.
     */
    @Bridge(symbol="CFHostGetReachability", optional=true)
    protected native NSData getReachability(BytePtr hasBeenResolved);
    /**
     * @since Available in iOS 2.0 and later.
     */
    @Bridge(symbol="CFHostCancelInfoResolution", optional=true)
    public native void cancelInfoResolution(CFHostInfoType info);
    /**
     * @since Available in iOS 2.0 and later.
     */
    @Bridge(symbol="CFHostSetClient", optional=true)
    protected native boolean setCallback(FunctionPtr clientCB, CFHostClientContext clientContext);
    /**
     * @since Available in iOS 2.0 and later.
     */
    @Bridge(symbol="CFHostScheduleWithRunLoop", optional=true)
    public native void schedule(CFRunLoop runLoop, CFString runLoopMode);
    /**
     * @since Available in iOS 2.0 and later.
     */
    @Bridge(symbol="CFHostUnscheduleFromRunLoop", optional=true)
    public native void unschedule(CFRunLoop runLoop, CFString runLoopMode);
    /**
     * @since Available in iOS 2.0 and later.
     */
    @Bridge(symbol="CFStreamCreatePairWithSocketToCFHost", optional=true)
    protected static native void createSocketStreamPair(CFAllocator alloc, CFHost host, int port, CFReadStream.CFReadStreamPtr readStream, CFWriteStream.CFWriteStreamPtr writeStream);
    /*</methods>*/
}
