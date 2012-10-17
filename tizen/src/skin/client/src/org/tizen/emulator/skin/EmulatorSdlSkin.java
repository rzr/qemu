/**
 * Emulator Skin Process
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

package org.tizen.emulator.skin;

import java.util.logging.Level;
import java.util.logging.Logger;

import org.eclipse.swt.SWT;
import org.tizen.emulator.skin.config.EmulatorConfig;
import org.tizen.emulator.skin.exception.ScreenShotException;
import org.tizen.emulator.skin.image.ImageRegistry.IconName;
import org.tizen.emulator.skin.log.SkinLogger;
import org.tizen.emulator.skin.mode.SkinMode;
import org.tizen.emulator.skin.screenshot.SdlScreenShotWindow;
import org.tizen.emulator.skin.util.SkinUtil;

public class EmulatorSdlSkin extends EmulatorSkin {
	private Logger logger = SkinLogger.getSkinLogger(
			EmulatorSdlSkin.class).getLogger();

	/**
	 *  Constructor
	 */
	public EmulatorSdlSkin(EmulatorConfig config, SkinMode mode, boolean isOnTop) {
		super(config, mode, isOnTop);
	}

	protected void openScreenShotWindow() {
		if (screenShotDialog != null) {
			return;
		}

		try {
			screenShotDialog = new SdlScreenShotWindow(shell, communicator, this, config,
					imageRegistry.getIcon(IconName.SCREENSHOT));
			screenShotDialog.open();

		} catch (ScreenShotException ex) {
			logger.log(Level.SEVERE, ex.getMessage(), ex);
			SkinUtil.openMessage(shell, null,
					"Fail to create a screen shot.", SWT.ICON_ERROR, config);

		} catch (Exception ex) {
			// defense exception handling.
			logger.log(Level.SEVERE, ex.getMessage(), ex);
			String errorMessage = "Internal Error.\n[" + ex.getMessage() + "]";
			SkinUtil.openMessage(shell, null, errorMessage, SWT.ICON_ERROR, config);

		} finally {
			screenShotDialog = null;
		}
	}

}
