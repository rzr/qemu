/**
 * Pop-up Menu
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

import java.util.logging.Logger;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.MenuItem;
import org.eclipse.swt.widgets.Shell;
import org.tizen.emulator.skin.EmulatorSkin;
import org.tizen.emulator.skin.config.EmulatorConfig;
import org.tizen.emulator.skin.dbi.MenuItemType;
import org.tizen.emulator.skin.dbi.PopupMenuType;
import org.tizen.emulator.skin.image.ImageRegistry;
import org.tizen.emulator.skin.image.ImageRegistry.IconName;
import org.tizen.emulator.skin.log.SkinLogger;
import org.tizen.emulator.skin.util.SkinUtil;
import org.tizen.emulator.skin.util.SwtUtil;

public class PopupMenu {
	private static Logger logger =
			SkinLogger.getSkinLogger(PopupMenu.class).getLogger();

	private EmulatorConfig config;
	private EmulatorSkin skin;
	private Shell shell;
	private ImageRegistry imageRegistry;
	private Menu contextMenu;

	/* default menu items */
	public MenuItem detailInfoItem;
	public MenuItem onTopItem;
	public MenuItem rotateItem;
	public MenuItem scaleItem;
	public MenuItem keyWindowItem; /* key window menu */
	public MenuItem advancedItem;
	public MenuItem screenshotItem;
	public MenuItem hostKeyboardItem;
	public MenuItem kbdOnItem;
	public MenuItem kbdOffItem;
	public MenuItem diagnosisItem;
	public MenuItem shellItem;
	public MenuItem closeItem;
	public MenuItem aboutItem;
	public MenuItem forceCloseItem;

	public PopupMenu(EmulatorConfig config, EmulatorSkin skin,
			Shell shell, ImageRegistry imageRegistry) {
		this.config = config;
		this.skin = skin;
		this.shell = shell;
		this.imageRegistry = imageRegistry;

		createMenu();
	}

	private void createMenu() {
		logger.info("create a popup menu");

		contextMenu = new Menu(shell);

		addMenuItems(contextMenu);
		shell.setMenu(contextMenu);
	}

	private void addMenuItems(final Menu menu) {
		PopupMenuType itemProperties = config.getDbiContents().getPopupMenu();
		String menuName = "";

		/* Emulator detail info menu */
		detailInfoItem = new MenuItem(menu, SWT.PUSH);
		String emulatorName = SkinUtil.makeEmulatorName(config);
		detailInfoItem.setText(emulatorName);
		detailInfoItem.setImage(imageRegistry.getIcon(IconName.DETAIL_INFO));

		SelectionAdapter detailInfoListener = skin.createDetailInfoMenu();
		detailInfoItem.addSelectionListener(detailInfoListener);

		new MenuItem(menu, SWT.SEPARATOR);

		/* Always on top menu */
		if (SwtUtil.isMacPlatform() == false) { /* not supported on mac */
			onTopItem = new MenuItem(menu, SWT.CHECK);
			onTopItem.setText("&Always On Top");
			onTopItem.setSelection(skin.isOnTop);

			SelectionAdapter topMostListener = skin.createTopMostMenu();
			onTopItem.addSelectionListener(topMostListener);
		}

		/* Rotate menu */
		MenuItemType rotationItem = (itemProperties != null) ?
				itemProperties.getRotationItem() : null;

		menuName = (rotationItem != null) ?
				rotationItem.getItemName() : "&Rotate";

		if (rotationItem == null ||
				(rotationItem != null && rotationItem.isVisible() == true)) {
			rotateItem = new MenuItem(menu, SWT.CASCADE);
			rotateItem.setText(menuName);
			rotateItem.setImage(imageRegistry.getIcon(IconName.ROTATE));

			Menu rotateMenu = skin.createRotateMenu();
			rotateItem.setMenu(rotateMenu);
		}

		/* Scale menu */
		scaleItem = new MenuItem(menu, SWT.CASCADE);
		scaleItem.setText("&Scale");
		scaleItem.setImage(imageRegistry.getIcon(IconName.SCALE));

		Menu scaleMenu = skin.createScaleMenu();
		scaleItem.setMenu(scaleMenu);

		new MenuItem(menu, SWT.SEPARATOR);

		/* Key Window menu */
		MenuItemType keywindowItem = (itemProperties != null) ?
				itemProperties.getKeywindowItem() : null;

		menuName = (keywindowItem != null) ?
				keywindowItem.getItemName() : "&Key Window";

		if (keywindowItem == null ||
				(keywindowItem != null && keywindowItem.isVisible() == true)) {
			keyWindowItem = new MenuItem(menu, SWT.CHECK);
			keyWindowItem.setText("&Key Window");
			keyWindowItem.setSelection(skin.isKeyWindow);

			SelectionAdapter keyWindowListener = skin.createKeyWindowMenu();
			keyWindowItem.addSelectionListener(keyWindowListener);
		}

		/* Advanced menu */
		advancedItem = new MenuItem(menu, SWT.CASCADE);
		advancedItem.setText("Ad&vanced");
		advancedItem.setImage(imageRegistry.getIcon(IconName.ADVANCED));

		Menu advancedMenu = new Menu(menu.getShell(), SWT.DROP_DOWN);
		{
			/* Screen shot menu */
			screenshotItem = new MenuItem(advancedMenu, SWT.PUSH);
			screenshotItem.setText("&Screen Shot");
			screenshotItem.setImage(imageRegistry.getIcon(IconName.SCREENSHOT));

			SelectionAdapter screenshotListener = skin.createScreenshotMenu();
			screenshotItem.addSelectionListener(screenshotListener);

			/* VirtIO Keyboard Menu */
			hostKeyboardItem = new MenuItem(advancedMenu, SWT.CASCADE);
			hostKeyboardItem.setText("&Host Keyboard");
			hostKeyboardItem.setImage(imageRegistry.getIcon(IconName.HOST_KEYBOARD));

			Menu hostKeyboardMenu = new Menu(shell, SWT.DROP_DOWN);

			kbdOnItem = new MenuItem(hostKeyboardMenu, SWT.RADIO);
			kbdOnItem.setText("On");
			kbdOnItem.setSelection(skin.isOnKbd);

			kbdOffItem = new MenuItem(hostKeyboardMenu, SWT.RADIO);
			kbdOffItem.setText("Off");
			kbdOffItem.setSelection(!skin.isOnKbd);

			SelectionAdapter hostKeyboardListener = skin.createHostKeyboardMenu();
			kbdOnItem.addSelectionListener(hostKeyboardListener);
			kbdOffItem.addSelectionListener(hostKeyboardListener);

			hostKeyboardItem.setMenu(hostKeyboardMenu);

			/* Diagnosis menu */
			if (SwtUtil.isLinuxPlatform()) { //TODO: windows
				diagnosisItem = new MenuItem(advancedMenu, SWT.CASCADE);
				diagnosisItem.setText("&Diagnosis");
				diagnosisItem.setImage(imageRegistry.getIcon(IconName.DIAGNOSIS));

				Menu diagnosisMenu = skin.createDiagnosisMenu();
				diagnosisItem.setMenu(diagnosisMenu);
			}

			new MenuItem(advancedMenu, SWT.SEPARATOR);

			/* About menu */
			aboutItem = new MenuItem(advancedMenu, SWT.PUSH);
			aboutItem.setText("&About");
			aboutItem.setImage(imageRegistry.getIcon(IconName.ABOUT));

			SelectionAdapter aboutListener = skin.createAboutMenu();
			aboutItem.addSelectionListener(aboutListener);

			new MenuItem(advancedMenu, SWT.SEPARATOR);

			/* Force close menu */
			forceCloseItem = new MenuItem(advancedMenu, SWT.PUSH);
			forceCloseItem.setText("&Force Close");
			forceCloseItem.setImage(imageRegistry.getIcon(IconName.FORCE_CLOSE));

			SelectionAdapter forceCloseListener = skin.createForceCloseMenu();
			forceCloseItem.addSelectionListener(forceCloseListener);
		}
		advancedItem.setMenu(advancedMenu);

		/* Shell menu */
		shellItem = new MenuItem(menu, SWT.PUSH);
		shellItem.setText("S&hell");
		shellItem.setImage(imageRegistry.getIcon(IconName.SHELL));

		SelectionAdapter shellListener = skin.createShellMenu();
		shellItem.addSelectionListener(shellListener);

		new MenuItem(menu, SWT.SEPARATOR);

		/* Close menu */
		closeItem = new MenuItem(menu, SWT.PUSH);
		closeItem.setText("&Close");
		closeItem.setImage(imageRegistry.getIcon(IconName.CLOSE));

		SelectionAdapter closeListener = skin.createCloseMenu();
		closeItem.addSelectionListener(closeListener);
	}

	public Menu getMenu() {
		return contextMenu;
	}
}
