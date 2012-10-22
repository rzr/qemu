/**
 * 
 *
 * Copyright (C) 2011 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
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

package org.tizen.emulator.skin.mode;

/**
 *
 */
public enum SkinMode {
	GENERAL("general"),
	FULLSCREEN("fullscreen"), /* not used yet */
	DEFAULT("default"),
	CUSTOM("custom"); /* not used yet */

	private String value;

	SkinMode(String value) {
		this.value = value;
	}

	public String value() {
		return this.value;
	}
	
	public static SkinMode getValue(String val) {
		SkinMode[] values = SkinMode.values();
		for (int i = 0; i < values.length; i++) {
			if (values[i].value.equalsIgnoreCase(val) == true) {
				return values[i];
			}
		}

		return SkinMode.DEFAULT;
	}
}

