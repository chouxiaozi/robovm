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
package org.robovm.apple.imageio;

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
import org.robovm.apple.coregraphics.*;
/*</imports>*/

/*<javadoc>*/
/*</javadoc>*/
@Marshaler(CGImagePropertyIPTCData.Marshaler.class)
/*<annotations>*/@Library("ImageIO")/*</annotations>*/
/*<visibility>*/public/*</visibility>*/ class CGImagePropertyIPTCData 
    extends /*<extends>*/Object/*</extends>*/ 
    /*<implements>*//*</implements>*/ {

    public static class Marshaler {
        @MarshalsPointer
        public static CGImagePropertyIPTCData toObject(Class<CGImagePropertyIPTCData> cls, long handle, long flags) {
            CFDictionary o = (CFDictionary) CFType.Marshaler.toObject(CFDictionary.class, handle, flags);
            if (o == null) {
                return null;
            }
            return new CGImagePropertyIPTCData(o);
        }
        @MarshalsPointer
        public static long toNative(CGImagePropertyIPTCData o, long flags) {
            if (o == null) {
                return 0L;
            }
            return CFType.Marshaler.toNative(o.data, flags);
        }
    }
    
    /*<ptr>*/
    /*</ptr>*/
    private CFDictionary data;
    
    protected CGImagePropertyIPTCData(CFDictionary data) {
        this.data = data;
    }
    public CGImagePropertyIPTCData() {
        this.data = CFMutableDictionary.create();
    }
    /*<bind>*/static { Bro.bind(CGImagePropertyIPTCData.class); }/*</bind>*/
    /*<constants>*//*</constants>*/
    /*<constructors>*//*</constructors>*/
    /*<properties>*//*</properties>*/
    /*<members>*//*</members>*/
    public CFDictionary getDictionary() {
        return data;
    }
    
    public boolean has(CGImagePropertyIPTC property) {
        return data.containsKey(property.value());
    }
    
    public String getString(CGImagePropertyIPTC property) {
        if (has(property)) {
            CFString val = data.get(property.value(), CFString.class);
            return val.toString();
        }
        return null;
    }
    public double getNumber(CGImagePropertyIPTC property) {
        if (has(property)) {
            CFNumber val = data.get(property.value(), CFNumber.class);
            return val.doubleValue();
        }
        return 0;
    }
    public CGImagePropertyIPTCData set(CGImagePropertyIPTC property, String value) {
        data.put(property.value(), new CFString(value));
        return this;
    }
    public CGImagePropertyIPTCData set(CGImagePropertyIPTC property, double value) {
        data.put(property.value(), CFNumber.valueOf(value));
        return this;
    }
    
    
    public CGImagePropertyIPTCContactInfoData getContactInfo() {
        if (has(CGImagePropertyIPTC.CreatorContactInfo)) {
            CFDictionary val = data.get(CGImagePropertyIPTC.CreatorContactInfoKey(), CFDictionary.class);
            return new CGImagePropertyIPTCContactInfoData(val);
        }
        return null;
    }
    public CGImagePropertyIPTCData setContactInfo(CGImagePropertyIPTCContactInfoData contactInfo) {
        data.put(CGImagePropertyIPTC.CreatorContactInfoKey(), contactInfo.getDictionary());
        return this;
    }
    /*<methods>*/
    /*</methods>*/
    @Override
    public String toString() {
        if (data != null) return data.toString();
        return super.toString();
    }
}
