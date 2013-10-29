/**
 * Hardware Key
 *
 * Copyright (C) 2011 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
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

package org.tizen.emulator.skin.layout;

import org.tizen.emulator.skin.util.HWKeyRegion;
import org.tizen.emulator.skin.util.SkinUtil;

public class HWKey {
	private String name;
	private int keyCode;
	private HWKeyRegion region;
	private String tooltip;

	/**
	 *  Constructor
	 */
	public HWKey() {
		this.name = "unknown";
		this.keyCode = SkinUtil.UNKNOWN_KEYCODE;
	}

	public HWKey(String name, int keyCode, HWKeyRegion region, String tooltip) {
		this.name = name;
		this.keyCode = keyCode;
		this.region = region;
		this.tooltip = tooltip;
	}

	/* name */
	public String getName() {
		return name;
	}

	public void setName(String name) {
		this.name = name;
	}

	/* keycode */
	public int getKeyCode() {
		return keyCode;
	}

	public void setKeyCode(int keyCode) {
		this.keyCode = keyCode;
	}

	/* region */
	public HWKeyRegion getRegion() {
		return region;
	}

	public void setRegion(HWKeyRegion region) {
		this.region = region;
	}

	/* tooltip */
	public String getTooltip() {
		return tooltip;
	}

	public void setTooltip(String tooltip) {
		this.tooltip = tooltip;
	}
}
