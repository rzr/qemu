/**
 * 
 *
 * Copyright (C) 2011 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * GiWoong Kim <giwoong.kim@samsung.com>
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
 * HyunJun Son
 * Hyunjin Lee <hyunjin816.lee@samsung.com
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Contributors:
 * - S-Core Co., Ltd
 *
 */

package org.tizen.emulator.skin.util;

import java.util.HashMap;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.Map.Entry;

import org.tizen.emulator.skin.comm.ICommunicator.RotationInfo;
import org.tizen.emulator.skin.dbi.RotationType;


/**
 * 
 *
 */
public class KeyWindowRotation {
	
	private static Map<Short, RotationType> rotationMap;
	private static Map<Short, RotationInfo> angleMap;
	
	private KeyWindowRotation(){}
	
	static {
		rotationMap = new LinkedHashMap<Short, RotationType>();
		angleMap = new HashMap<Short, RotationInfo>();
	}
	
	public static void clear() {
		if (!rotationMap.isEmpty())
			rotationMap.clear();
		if (!angleMap.isEmpty())
			angleMap.clear();
	}
	
	public static void put(RotationType rotation ) {

		if ( RotationInfo.PORTRAIT.value().equalsIgnoreCase( rotation.getName().value() ) ) {
			rotationMap.put( RotationInfo.PORTRAIT.id(), rotation );
			angleMap.put( RotationInfo.PORTRAIT.id(), RotationInfo.PORTRAIT );
		} else if ( RotationInfo.LANDSCAPE.value().equalsIgnoreCase( rotation.getName().value() ) ) {
			rotationMap.put( RotationInfo.LANDSCAPE.id(), rotation );
			angleMap.put( RotationInfo.LANDSCAPE.id(), RotationInfo.LANDSCAPE );
		} else if ( RotationInfo.REVERSE_PORTRAIT.value().equalsIgnoreCase( rotation.getName().value() ) ) {
			rotationMap.put( RotationInfo.REVERSE_PORTRAIT.id(), rotation );
			angleMap.put( RotationInfo.REVERSE_PORTRAIT.id(), RotationInfo.REVERSE_PORTRAIT );
		} else if ( RotationInfo.REVERSE_LANDSCAPE.value().equalsIgnoreCase( rotation.getName().value() ) ) {
			rotationMap.put( RotationInfo.REVERSE_LANDSCAPE.id(), rotation );
			angleMap.put( RotationInfo.REVERSE_LANDSCAPE.id(), RotationInfo.REVERSE_LANDSCAPE );
		}

	}

	public static int getAngle( Short rotationId ) {
		RotationInfo rotationInfo = angleMap.get(rotationId);
		if( null != rotationInfo ) {
			return rotationInfo.angle();
		}else {
			return 0;
		}
	}

	public static RotationType getRotation( Short rotationId ) {
		return rotationMap.get(rotationId);
	}

	public static Iterator<Entry<Short, RotationType>>getRotationIterator() {
		return rotationMap.entrySet().iterator();
	}
	
}
