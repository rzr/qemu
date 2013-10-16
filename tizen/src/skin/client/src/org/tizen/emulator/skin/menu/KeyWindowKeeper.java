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
import org.eclipse.swt.widgets.MenuItem;
import org.tizen.emulator.skin.EmulatorSkin;
import org.tizen.emulator.skin.custom.GeneralKeyWindow;
import org.tizen.emulator.skin.custom.SkinWindow;
import org.tizen.emulator.skin.custom.SpecialKeyWindow;
import org.tizen.emulator.skin.dbi.KeyMapType;
import org.tizen.emulator.skin.image.GeneralKeyWindowImageRegistry;
import org.tizen.emulator.skin.log.SkinLogger;
import org.tizen.emulator.skin.util.SkinUtil;

public class KeyWindowKeeper {
	private static Logger logger =
			SkinLogger.getSkinLogger(KeyWindowKeeper.class).getLogger();

	private EmulatorSkin skin;
	private SkinWindow keyWindow;
	private int recentlyDocked;
	private int indexLayout;

	private GeneralKeyWindowImageRegistry imageRegstry;

	/**
	 *  Constructor
	 */
	public KeyWindowKeeper(EmulatorSkin skin) {
		this.skin = skin;
		this.recentlyDocked = SWT.NONE;
		this.indexLayout = -1;
		this.imageRegstry = null;
	}

	public void openKeyWindow(int dockValue, boolean recreate) {
		if (keyWindow != null) {
			if (recreate == false) {
				/* show the Key Window */
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

		/* create a Key Window */
		determineLayout();

		if (isGeneralKeyWindow() == true) {
			if (imageRegstry == null) {
				logger.warning("GeneralKeyWindowImageRegistry is null");
				return;
			}

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

			keyWindow = new GeneralKeyWindow(skin, imageRegstry, keyMapList);
		} else {
			// TODO:
			String layoutName =
					skin.getPopupMenu().keyWindowItem.getMenu().getItem(indexLayout).getText();
			logger.info("generate a \'" + layoutName + "\' key window!");

			keyWindow = new SpecialKeyWindow(skin, layoutName);
		}

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

	public int getLayoutIndex() {
		return indexLayout;
	}

	public boolean isGeneralKeyWindow() {
		return (getLayoutIndex() < 0);
	}

	public int determineLayout() {
		MenuItem keywindowItem = skin.getPopupMenu().keyWindowItem;

		if (keywindowItem != null && keywindowItem.getMenu() != null) {
			logger.info("key window has a special layout");

			MenuItem[] layouts = keywindowItem.getMenu().getItems();
			for (int i = 0; i < layouts.length; i++) {
				MenuItem layout = layouts[i];

				if (layout.getSelection() == true) {
					indexLayout = i;

					logger.info("the \'" + layout.getText() +
							"\' layout is selected for key window");
					break;
				}
			}
		} else {
			logger.info("key window has a general layout");

			indexLayout = -1;

			if (imageRegstry == null) {
				imageRegstry = new GeneralKeyWindowImageRegistry(
						skin.getShell().getDisplay());
			}
		}

		return indexLayout;
	}

	/* for Menu */
	public boolean isSelectKeyWindowMenu() {
		MenuItem keywindow = skin.getPopupMenu().keyWindowItem;

		if (keywindow != null) {
			if (isGeneralKeyWindow() == true) {
				return keywindow.getSelection();
			} else {
				for (MenuItem layout : keywindow.getMenu().getItems()) {
					if (layout.getSelection() == true) {
						return true;
					}
				}
			}
		}

		return false;
	}

	public void selectKeyWindowMenu(boolean on) {
		MenuItem keywindow = skin.getPopupMenu().keyWindowItem;

		if (keywindow != null) {
			if (isGeneralKeyWindow() == true) {
				keywindow.setSelection(on);
			} else {
				keywindow.getMenu().getItem(indexLayout).setSelection(on);
			}
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

	public void setRecentlyDocked(int dockValue) {
		recentlyDocked = dockValue;
	}

	public void dispose() {
		closeKeyWindow();

		if (imageRegstry != null) {
			imageRegstry.dispose();
		}
	}
}
