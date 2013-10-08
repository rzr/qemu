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

import java.io.File;
import java.io.FileFilter;
import java.util.ArrayList;
import java.util.logging.Logger;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.MenuItem;
import org.eclipse.swt.widgets.Shell;
import org.tizen.emulator.skin.EmulatorSkin;
import org.tizen.emulator.skin.config.EmulatorConfig;
import org.tizen.emulator.skin.custom.SpecialKeyWindow;
import org.tizen.emulator.skin.dbi.MenuItemType;
import org.tizen.emulator.skin.dbi.PopupMenuType;
import org.tizen.emulator.skin.image.ImageRegistry;
import org.tizen.emulator.skin.image.ImageRegistry.IconName;
import org.tizen.emulator.skin.log.SkinLogger;
import org.tizen.emulator.skin.util.SkinUtil;
import org.tizen.emulator.skin.util.SwtUtil;

public class PopupMenu {
	public static final String ECP_NAME = "Control &Panel";
	public static final String TOPMOST_MENUITEM_NAME = "&Always On Top";
	public static final String ROTATE_MENUITEM_NAME = "&Rotate";
	public static final String SCALE_MENUITEM_NAME = "&Scale";
	public static final String KEYWINDOW_MENUITEM_NAME = "&Key Window";
	public static final String ADVANCED_MENUITEM_NAME = "Ad&vanced";
	public static final String SCREENSHOT_MENUITEM_NAME = "&Screen Shot";
	public static final String HOSTKEYBOARD_MENUITEM_NAME = "&Host Keyboard";
	public static final String DIAGNOSIS_MENUITEM_NAME = "&Diagnosis";
	public static final String RAMDUMP_MENUITEM_NAME = "&Ram Dump";
	public static final String ABOUT_MENUITEM_NAME = "&About";
	public static final String FORCECLOSE_MENUITEM_NAME = "&Force Close";
	public static final String SDBSHELL_MENUITEM_NAME = "S&hell";
	public static final String CLOSE_MENUITEM_NAME = "&Close";

	private static Logger logger =
			SkinLogger.getSkinLogger(PopupMenu.class).getLogger();

	private EmulatorConfig config;
	private EmulatorSkin skin;
	private Shell shell;
	private ImageRegistry imageRegistry;
	private Menu contextMenu;

	/* default menu items */
	public MenuItem detailInfoItem;
	public MenuItem ecpItem;
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
	public MenuItem ramdumpItem;
	public MenuItem aboutItem;
	public MenuItem forceCloseItem;
	public MenuItem shellItem;
	public MenuItem closeItem;

	/**
	 *  Constructor
	 */
	public PopupMenu(EmulatorConfig config, EmulatorSkin skin) {
		this.config = config;
		this.skin = skin;
		this.shell = skin.getShell();
		this.imageRegistry = skin.getImageRegistry();

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
		String menuName = "N/A";

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
			MenuItemType topmostMenuType = (itemProperties != null) ?
					itemProperties.getTopmostItem() : null;

			menuName = (topmostMenuType != null &&
					topmostMenuType.getItemName().isEmpty() == false) ?
							topmostMenuType.getItemName() : TOPMOST_MENUITEM_NAME;

			if (topmostMenuType == null ||
					(topmostMenuType != null && topmostMenuType.isVisible() == true)) {
				onTopItem = new MenuItem(menu, SWT.CHECK);
				onTopItem.setText(menuName);
				onTopItem.setSelection(skin.isOnTop);

				SelectionAdapter topMostListener = skin.createTopMostMenu();
				onTopItem.addSelectionListener(topMostListener);
			}
		}

		/* Rotate menu */
		MenuItemType rotationMenuType = (itemProperties != null) ?
				itemProperties.getRotateItem() : null;

		menuName = (rotationMenuType != null &&
				rotationMenuType.getItemName().isEmpty() == false) ?
						rotationMenuType.getItemName() : ROTATE_MENUITEM_NAME;

		if (rotationMenuType == null ||
				(rotationMenuType != null && rotationMenuType.isVisible() == true)) {
			rotateItem = new MenuItem(menu, SWT.CASCADE);
			rotateItem.setText(menuName);
			rotateItem.setImage(imageRegistry.getIcon(IconName.ROTATE));

			Menu rotateSubMenu = skin.createRotateMenu();
			rotateItem.setMenu(rotateSubMenu);
		}

		/* Scale menu */
		MenuItemType scaleMenuType = (itemProperties != null) ?
				itemProperties.getScaleItem() : null;

		menuName = (scaleMenuType != null &&
				scaleMenuType.getItemName().isEmpty() == false) ?
						scaleMenuType.getItemName() : SCALE_MENUITEM_NAME;

		if (scaleMenuType == null ||
				(scaleMenuType != null && scaleMenuType.isVisible() == true)) {
			scaleItem = new MenuItem(menu, SWT.CASCADE);
			scaleItem.setText(menuName);
			scaleItem.setImage(imageRegistry.getIcon(IconName.SCALE));

			Menu scaleSubMenu = skin.createScaleMenu();
			scaleItem.setMenu(scaleSubMenu);
		}

		new MenuItem(menu, SWT.SEPARATOR);

		/* Key Window menu */
		MenuItemType keywindowMenuType = (itemProperties != null) ?
				itemProperties.getKeywindowItem() : null;

		menuName = (keywindowMenuType != null &&
				keywindowMenuType.getItemName().isEmpty() == false) ?
						keywindowMenuType.getItemName() : KEYWINDOW_MENUITEM_NAME;

		if (keywindowMenuType == null ||
				(keywindowMenuType != null && keywindowMenuType.isVisible() == true)) {
			/* load Key Window layout */
			String pathLayoutRoot = skin.skinInfo.getSkinPath() +
					File.separator + SpecialKeyWindow.KEYWINDOW_LAYOUT_ROOT;
			ArrayList<File> layouts = getKeyWindowLayoutList(pathLayoutRoot);

			if (layouts != null) {
				keyWindowItem = new MenuItem(menu, SWT.CASCADE);
				keyWindowItem.setText(menuName);

				Menu keywindowSubMenu = new Menu(menu.getShell(), SWT.DROP_DOWN);
				{
					MenuItem keywindowLayoutItem = null;

					for (int i = 0; i < layouts.size(); i++) {
						File dir = layouts.get(i);

						keywindowLayoutItem = new MenuItem(keywindowSubMenu, SWT.CHECK);
						keywindowLayoutItem.setText(dir.getName());
						if (i == 0) {
							keywindowLayoutItem.setSelection(true);
						}

						SelectionAdapter keywindowLayoutListener = skin.createKeyWindowMenu();
						keywindowLayoutItem.addSelectionListener(keywindowLayoutListener);
					}
				}
				keyWindowItem.setMenu(keywindowSubMenu);
			} else {
				keyWindowItem = new MenuItem(menu, SWT.CHECK);
				keyWindowItem.setText(menuName);
				keyWindowItem.setSelection(skin.isKeyWindow);

				SelectionAdapter keyWindowListener = skin.createKeyWindowMenu();
				keyWindowItem.addSelectionListener(keyWindowListener);
			}
		}

		/* Advanced menu */
		advancedItem = new MenuItem(menu, SWT.CASCADE);
		advancedItem.setText(ADVANCED_MENUITEM_NAME);
		advancedItem.setImage(imageRegistry.getIcon(IconName.ADVANCED));

		Menu advancedSubMenu = new Menu(menu.getShell(), SWT.DROP_DOWN);
		{
			/* Screen shot menu */
			screenshotItem = new MenuItem(advancedSubMenu, SWT.PUSH);
			screenshotItem.setText(SCREENSHOT_MENUITEM_NAME);
			screenshotItem.setImage(imageRegistry.getIcon(IconName.SCREENSHOT));

			SelectionAdapter screenshotListener = skin.createScreenshotMenu();
			screenshotItem.addSelectionListener(screenshotListener);

			/* VirtIO Keyboard Menu */
			hostKeyboardItem = new MenuItem(advancedSubMenu, SWT.CASCADE);
			hostKeyboardItem.setText(HOSTKEYBOARD_MENUITEM_NAME);
			hostKeyboardItem.setImage(imageRegistry.getIcon(IconName.HOST_KEYBOARD));

			Menu hostKeyboardSubMenu = new Menu(menu.getShell(), SWT.DROP_DOWN);
			{
				kbdOnItem = new MenuItem(hostKeyboardSubMenu, SWT.RADIO);
				kbdOnItem.setText("On");
				kbdOnItem.setSelection(skin.isOnKbd);

				kbdOffItem = new MenuItem(hostKeyboardSubMenu, SWT.RADIO);
				kbdOffItem.setText("Off");
				kbdOffItem.setSelection(!skin.isOnKbd);

				SelectionAdapter hostKeyboardListener = skin.createHostKeyboardMenu();
				kbdOnItem.addSelectionListener(hostKeyboardListener);
				kbdOffItem.addSelectionListener(hostKeyboardListener);
			}
			hostKeyboardItem.setMenu(hostKeyboardSubMenu);

			/* Diagnosis menu */
			if (SwtUtil.isLinuxPlatform()) { //TODO: windows
				diagnosisItem = new MenuItem(advancedSubMenu, SWT.CASCADE);
				diagnosisItem.setText(DIAGNOSIS_MENUITEM_NAME);
				diagnosisItem.setImage(imageRegistry.getIcon(IconName.DIAGNOSIS));

				Menu diagnosisSubMenu = new Menu(menu.getShell(), SWT.DROP_DOWN);
				{
					ramdumpItem = new MenuItem(diagnosisSubMenu, SWT.PUSH);
					ramdumpItem.setText(RAMDUMP_MENUITEM_NAME);

					SelectionAdapter ramdumpListener = skin.createRamdumpMenu();
					ramdumpItem.addSelectionListener(ramdumpListener);
				}
				diagnosisItem.setMenu(diagnosisSubMenu);
			}

			new MenuItem(advancedSubMenu, SWT.SEPARATOR);

			/* About menu */
			aboutItem = new MenuItem(advancedSubMenu, SWT.PUSH);
			aboutItem.setText(ABOUT_MENUITEM_NAME);
			aboutItem.setImage(imageRegistry.getIcon(IconName.ABOUT));

			SelectionAdapter aboutListener = skin.createAboutMenu();
			aboutItem.addSelectionListener(aboutListener);

			new MenuItem(advancedSubMenu, SWT.SEPARATOR);

			/* Force close menu */
			forceCloseItem = new MenuItem(advancedSubMenu, SWT.PUSH);
			forceCloseItem.setText(FORCECLOSE_MENUITEM_NAME);
			forceCloseItem.setImage(imageRegistry.getIcon(IconName.FORCE_CLOSE));

			SelectionAdapter forceCloseListener = skin.createForceCloseMenu();
			forceCloseItem.addSelectionListener(forceCloseListener);
		}
		advancedItem.setMenu(advancedSubMenu);

		/* Shell menu */
		MenuItemType shellMenuType = (itemProperties != null) ?
				itemProperties.getShellItem() : null;

		menuName = (shellMenuType != null &&
				shellMenuType.getItemName().isEmpty() == false) ?
						shellMenuType.getItemName() : SDBSHELL_MENUITEM_NAME;

		if (shellMenuType == null ||
				(shellMenuType != null && shellMenuType.isVisible() == true)) {
			shellItem = new MenuItem(menu, SWT.PUSH);
			shellItem.setText(menuName);
			shellItem.setImage(imageRegistry.getIcon(IconName.SHELL));

			SelectionAdapter shellListener = skin.createShellMenu();
			shellItem.addSelectionListener(shellListener);
		}

		new MenuItem(menu, SWT.SEPARATOR);

		ecpItem = new MenuItem(menu, SWT.PUSH);
		{
			ecpItem.setText(ECP_NAME);
			ecpItem.setImage(imageRegistry.getIcon(IconName.ECP));

			SelectionAdapter ecpListener = skin.createEcpMenu();
			ecpItem.addSelectionListener(ecpListener);
		}
		
		new MenuItem(menu, SWT.SEPARATOR);
		
		/* Close menu */
		closeItem = new MenuItem(menu, SWT.PUSH);
		closeItem.setText(CLOSE_MENUITEM_NAME);
		closeItem.setImage(imageRegistry.getIcon(IconName.CLOSE));

		SelectionAdapter closeListener = skin.createCloseMenu();
		closeItem.addSelectionListener(closeListener);
	}

	public Menu getMenuRoot() {
		return contextMenu;
	}

	private ArrayList<File> getKeyWindowLayoutList(String path) {
		File pathRoot = new File(path);

		if (pathRoot.exists() == true) {
			FileFilter filter = new FileFilter() {
				@Override
				public boolean accept(File pathname) {
					if (!pathname.isDirectory()) {
						return false;
					}

					return true;
				}
			};

			File[] layoutPaths = pathRoot.listFiles(filter);
			if (layoutPaths.length <= 0) {
				logger.info("the layout of key window not found");
				return null;
			}

			ArrayList<File> layoutList = new ArrayList<File>();
			for (File layout : layoutPaths) {
				logger.info("the layout of key window detected : " + layout.getName());

				layoutList.add(layout);
			}

			return layoutList;
		}

		return null;
	}
}
