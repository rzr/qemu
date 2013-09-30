/**
 * Skin Information
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

package org.tizen.emulator.skin.info;

import java.util.logging.Logger;

import org.tizen.emulator.skin.log.SkinLogger;

/*
 * 
 */
public class SkinInformation {
	private static Logger logger =
			SkinLogger.getSkinLogger(SkinInformation.class).getLogger();

	private String skinName;
	private String skinPath;
	private boolean isGeneralSkin;
	private int skinOption;

	/**
	 *  Constructor
	 */
	public SkinInformation(String skinName, String skinPath, boolean isGeneralSkin) {
		this.skinName = skinName;
		this.skinPath = skinPath;
		this.isGeneralSkin = isGeneralSkin;
		this.skinOption = 0;

		if (isGeneralPurposeSkin() == true) {
			logger.info("This skin has a general purpose layout");
		} else {
			logger.info("This skin has a profile specific layout");
		}
	}

	public String getSkinName() {
		return skinName;
	}

	public String getSkinPath() {
		return skinPath;
	}

	public boolean isGeneralPurposeSkin() {
		return isGeneralSkin;
	}

	public int getSkinOption() {
		return skinOption;
	}

	public void setSkinOption(int option) {
		this.skinOption = option;
	}
}
