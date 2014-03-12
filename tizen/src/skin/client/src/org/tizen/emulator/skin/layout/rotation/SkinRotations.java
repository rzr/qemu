/**
 * Rotation Informations
 *
 * Copyright (C) 2011 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * GiWoong Kim <giwoong.kim@samsung.com>
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
 * HyunJun Son
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

package org.tizen.emulator.skin.layout.rotation;

import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.Map.Entry;

import org.tizen.emulator.skin.dbi.RotationNameType;
import org.tizen.emulator.skin.dbi.RotationType;

/**
 * 
 *
 */
public class SkinRotations {
	public static final short PORTRAIT_ID = 0;
	public static final short LANDSCAPE_ID = 1;
	public static final short REVERSE_PORTRAIT_ID = 2;
	public static final short REVERSE_LANDSCAPE_ID = 3;

	private static Map<Short, Rotation> rotationMap =
			new LinkedHashMap<Short, Rotation>();

	private SkinRotations() {
		/* do nothing */
	}

	public static void put(RotationType rotationType) {
		Rotation rotation = new Rotation();
		rotation.setName(rotationType.getName());
		rotation.setDisplay(rotationType.getDisplay());
		rotation.setImageList(rotationType.getImageList());
		rotation.setKeyMapList(rotationType.getKeyMapList());

		if (RotationNameType.PORTRAIT.value()
				.equalsIgnoreCase(rotationType.getName().value()))
		{
			rotation.setAngle(0);
			rotationMap.put(PORTRAIT_ID, rotation);
		}
		else if (RotationNameType.LANDSCAPE.value()
				.equalsIgnoreCase(rotationType.getName().value()))
		{
			rotation.setAngle(-90);
			rotationMap.put(LANDSCAPE_ID, rotation);
		}
		else if (RotationNameType.REVERSE_PORTRAIT.value()
				.equalsIgnoreCase(rotationType.getName().value()))
		{
			rotation.setAngle(180);
			rotationMap.put(REVERSE_PORTRAIT_ID, rotation);
		}
		else if (RotationNameType.REVERSE_LANDSCAPE.value()
				.equalsIgnoreCase(rotationType.getName().value()))
		{
			rotation.setAngle(90);
			rotationMap.put(REVERSE_LANDSCAPE_ID, rotation);
		}
	}

	public static Rotation getRotation(Short rotationId) {
		return rotationMap.get(rotationId);
	}

	public static int getAngle(Short rotationId) {
		Rotation rotation = rotationMap.get(rotationId);
		if (rotation == null) {
			return 0;
		}

		return rotation.getAngle();
	}

	public static Iterator<Entry<Short, Rotation>> getRotationIterator() {
		return rotationMap.entrySet().iterator();
	}
}
