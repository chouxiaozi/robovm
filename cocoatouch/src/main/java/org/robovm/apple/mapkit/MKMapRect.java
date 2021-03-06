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
package org.robovm.apple.mapkit;

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
import org.robovm.apple.corelocation.*;
import org.robovm.apple.uikit.*;
import org.robovm.apple.dispatch.*;
/*</imports>*/

/*<javadoc>*/

/*</javadoc>*/
/*<annotations>*/@Library("MapKit")/*</annotations>*/
/*<visibility>*/public/*</visibility>*/ class /*<name>*/MKMapRect/*</name>*/ 
    extends /*<extends>*/Struct<MKMapRect>/*</extends>*/ 
    /*<implements>*//*</implements>*/ {

    /*<ptr>*/public static class MKMapRectPtr extends Ptr<MKMapRect, MKMapRectPtr> {}/*</ptr>*/
    /*<bind>*/static { Bro.bind(MKMapRect.class); }/*</bind>*/
    /*<constants>*//*</constants>*/
    /*<constructors>*/
    public MKMapRect() {}
    public MKMapRect(MKMapPoint origin, MKMapSize size) {
        this.setOrigin(origin);
        this.setSize(size);
    }
    /*</constructors>*/
    public MKMapRect(double x, double y, double width, double height) {
        this.setOrigin(new MKMapPoint(x, y));
        this.setSize(new MKMapSize(width, height));
    }
    /*<properties>*//*</properties>*/
    /*<members>*/
    @StructMember(0) public native @ByVal MKMapPoint getOrigin();
    @StructMember(0) public native MKMapRect setOrigin(@ByVal MKMapPoint origin);
    
    @Deprecated
    @StructMember(0) public native @ByVal MKMapPoint origin();
    @Deprecated
    @StructMember(0) public native MKMapRect origin(@ByVal MKMapPoint origin);
    
    @StructMember(1) public native @ByVal MKMapSize getSize();
    @StructMember(1) public native MKMapRect setSize(@ByVal MKMapSize size);
    
    @Deprecated
    @StructMember(1) public native @ByVal MKMapSize size();
    @Deprecated
    @StructMember(1) public native MKMapRect size(@ByVal MKMapSize size);
    
    /*</members>*/
    /*<methods>*/
    /**
     * @since Available in iOS 4.0 and later.
     */
    @GlobalValue(symbol="MKMapRectWorld", optional=true)
    public static native @ByVal MKMapRect getWorldRect();
    /**
     * @since Available in iOS 4.0 and later.
     */
    @GlobalValue(symbol="MKMapRectNull", optional=true)
    public static native @ByVal MKMapRect getNullRect();
    
    /**
     * @since Available in iOS 4.0 and later.
     */
    public MKMapRect union(MKMapRect rect2) { return union(this, rect2); }
    @Bridge(symbol="MKMapRectUnion", optional=true)
    private static native @ByVal MKMapRect union(@ByVal MKMapRect rect1, @ByVal MKMapRect rect2);
    /**
     * @since Available in iOS 4.0 and later.
     */
    public MKMapRect intersection(MKMapRect rect2) { return intersection(this, rect2); }
    @Bridge(symbol="MKMapRectIntersection", optional=true)
    private static native @ByVal MKMapRect intersection(@ByVal MKMapRect rect1, @ByVal MKMapRect rect2);
    /**
     * @since Available in iOS 4.0 and later.
     */
    public MKMapRect inset(double dx, double dy) { return inset(this, dx, dy); }
    @Bridge(symbol="MKMapRectInset", optional=true)
    private static native @ByVal MKMapRect inset(@ByVal MKMapRect rect, double dx, double dy);
    /**
     * @since Available in iOS 4.0 and later.
     */
    public MKMapRect offset(double dx, double dy) { return offset(this, dx, dy); }
    @Bridge(symbol="MKMapRectOffset", optional=true)
    private static native @ByVal MKMapRect offset(@ByVal MKMapRect rect, double dx, double dy);
    /**
     * @since Available in iOS 4.0 and later.
     */
    public void divide(MKMapRect slice, MKMapRect remainder, double amount, CGRectEdge edge) { divide(this, slice, remainder, amount, edge); }
    @Bridge(symbol="MKMapRectDivide", optional=true)
    private static native void divide(@ByVal MKMapRect rect, MKMapRect slice, MKMapRect remainder, double amount, CGRectEdge edge);
    /**
     * @since Available in iOS 4.0 and later.
     */
    public boolean contains(MKMapPoint point) { return contains(this, point); }
    @Bridge(symbol="MKMapRectContainsPoint", optional=true)
    private static native boolean contains(@ByVal MKMapRect rect, @ByVal MKMapPoint point);
    /**
     * @since Available in iOS 4.0 and later.
     */
    public boolean contains(MKMapRect rect2) { return contains(this, rect2); }
    @Bridge(symbol="MKMapRectContainsRect", optional=true)
    private static native boolean contains(@ByVal MKMapRect rect1, @ByVal MKMapRect rect2);
    /**
     * @since Available in iOS 4.0 and later.
     */
    public boolean intersects(MKMapRect rect2) { return intersects(this, rect2); }
    @Bridge(symbol="MKMapRectIntersectsRect", optional=true)
    private static native boolean intersects(@ByVal MKMapRect rect1, @ByVal MKMapRect rect2);
    /**
     * @since Available in iOS 4.0 and later.
     */
    public boolean isSpanning180thMeridian() { return isSpanning180thMeridian(this); }
    @Bridge(symbol="MKMapRectSpans180thMeridian", optional=true)
    private static native boolean isSpanning180thMeridian(@ByVal MKMapRect rect);
    /**
     * @since Available in iOS 4.0 and later.
     */
    public MKMapRect remainder() { return remainder(this); }
    @Bridge(symbol="MKMapRectRemainder", optional=true)
    private static native @ByVal MKMapRect remainder(@ByVal MKMapRect rect);
    /*</methods>*/
}
