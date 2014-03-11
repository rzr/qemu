/**
 * Rotation Information
 *
 * Copyright (C) 2014 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * GiWoong Kim <giwoong.kim@samsung.com>
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
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

import java.util.List;

import org.tizen.emulator.skin.dbi.KeyMapListType;
import org.tizen.emulator.skin.dbi.KeyMapType;
import org.tizen.emulator.skin.dbi.RotationType;


public class Rotation extends RotationType {
	private int angle;
	private List<KeyMapType> listHWKey;

	public int getAngle() {
		return angle;
	}

	public void setAngle(int degree) {
		this.angle = degree;
	}

	public List<KeyMapType> getListHWKey() {
		if (listHWKey == null) {
			KeyMapListType list = getKeyMapList();
			if (list == null) {
				/* try to using a KeyMapList of portrait */
				Rotation rotation = SkinRotations.getRotation(SkinRotations.PORTRAIT_ID);
				if (rotation == null) {
					return null;
				}

				list = rotation.getKeyMapList();
				if (list == null) {
					return null;
				}
			}

			listHWKey = list.getKeyMap();
		}

		return listHWKey;
	}
}
