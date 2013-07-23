/**
 * Key Window Controller
 *
 * Copyright (C) 2013 Samsung Electronics Co., Ltd. All rights reserved.
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

package org.tizen.emulator.skin.menu;

import java.util.List;
import java.util.logging.Logger;

import org.eclipse.swt.SWT;
import org.tizen.emulator.skin.EmulatorSkin;
import org.tizen.emulator.skin.custom.KeyWindow;
import org.tizen.emulator.skin.custom.SkinWindow;
import org.tizen.emulator.skin.dbi.KeyMapType;
import org.tizen.emulator.skin.log.SkinLogger;
import org.tizen.emulator.skin.util.SkinUtil;

public class KeyWindowKeeper {
	private static Logger logger =
			SkinLogger.getSkinLogger(KeyWindowKeeper.class).getLogger();

	private EmulatorSkin skin;
	private SkinWindow keyWindow;
	private int recentlyDocked;

	/**
	 *  Constructor
	 */
	public KeyWindowKeeper(EmulatorSkin skin) {
		this.skin = skin;
		this.recentlyDocked = SWT.NONE;
	}

	public void openKeyWindow(int dockValue, boolean recreate) {
		if (keyWindow != null) {
			if (recreate == false) {
				/* show the key window */
				selectKeyWindowMenu(skin.isKeyWindow = true);

				if (skin.pairTag != null) {
					skin.pairTag.setVisible(true);
				}

				keyWindow.getShell().setVisible(true);
				SkinUtil.setTopMost(keyWindow.getShell(), skin.isOnTop);

				return;
			} else {
				logger.info("recreate a keywindow");
				closeKeyWindow();
			}
		}

		/* create a key window */
		List<KeyMapType> keyMapList = SkinUtil.getHWKeyMapList(
				skin.getEmulatorSkinState().getCurrentRotationId());

		if (keyMapList == null) {
			selectKeyWindowMenu(skin.isKeyWindow = false);
			logger.info("keyMapList is null");
			return;
		} else if (keyMapList.isEmpty() == true) {
			selectKeyWindowMenu(skin.isKeyWindow = false);
			logger.info("keyMapList is empty");
			return;
		}

		keyWindow = new KeyWindow(
				skin, skin.getShell(), skin.communicator, keyMapList);

		selectKeyWindowMenu(skin.isKeyWindow = true);
		SkinUtil.setTopMost(keyWindow.getShell(), skin.isOnTop);

		if (skin.pairTag != null) {
			skin.pairTag.setVisible(true);
		}

		keyWindow.open(dockValue);
	}

	public void closeKeyWindow() {
		selectKeyWindowMenu(skin.isKeyWindow = false);

		if (skin.pairTag != null) {
			skin.pairTag.setVisible(false);
		}

		if (keyWindow != null) {
			keyWindow.getShell().close();
			keyWindow = null;
		}
	}

	public void hideKeyWindow() {
		selectKeyWindowMenu(skin.isKeyWindow = false);

		if (skin.pairTag != null) {
			skin.pairTag.setVisible(false);
		}

		if (keyWindow != null) {
			keyWindow.getShell().setVisible(false);
		}
	}

	public SkinWindow getKeyWindow() {
		return keyWindow;
	}

	/* for Menu */
	public boolean isSelectKeyWindowMenu() {
		if (skin.getPopupMenu().keyWindowItem != null) {
			return skin.getPopupMenu().keyWindowItem.getSelection();
		}

		return false;
	}

	public void selectKeyWindowMenu(boolean on) {
		if (skin.getPopupMenu().keyWindowItem != null) {
			skin.getPopupMenu().keyWindowItem.setSelection(on);
		}
	}

	/* for docking */
	public void redock(boolean correction, boolean enableLogger) {
		if (keyWindow != null) {
			keyWindow.redock(correction, enableLogger);
		}
	}

	public int getDockPosition() {
		if (keyWindow == null) {
			return SWT.None;
		}

		return keyWindow.getDockPosition();
	}

	public int getRecentlyDocked() {
		return recentlyDocked;
	}

	public void setRecentlyDocked(int recentlyDocked) {
		this.recentlyDocked = recentlyDocked;
	}
}
