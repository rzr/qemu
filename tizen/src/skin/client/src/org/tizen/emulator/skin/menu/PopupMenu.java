/**
 * Right Click Popup Menu
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
import java.util.Arrays;
import java.util.List;
import java.util.logging.Logger;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.MenuItem;
import org.eclipse.swt.widgets.Shell;
import org.tizen.emulator.skin.EmulatorSkin;
import org.tizen.emulator.skin.comm.ICommunicator.RotationInfo;
import org.tizen.emulator.skin.config.EmulatorConfig;
import org.tizen.emulator.skin.dbi.MenuItemType;
import org.tizen.emulator.skin.dbi.PopupMenuType;
import org.tizen.emulator.skin.dbi.ScaleItemType;
import org.tizen.emulator.skin.image.ImageRegistry;
import org.tizen.emulator.skin.image.ImageRegistry.IconName;
import org.tizen.emulator.skin.log.SkinLogger;
import org.tizen.emulator.skin.util.SkinUtil;
import org.tizen.emulator.skin.util.SwtUtil;

public class PopupMenu {
	public static final String TOPMOST_MENUITEM_NAME = "&Always On Top";
	public static final String ROTATE_MENUITEM_NAME = "&Rotate";
	public static final String SCALE_MENUITEM_NAME = "&Scale";
	public static final String INTERPOLATION_MENUITEM_NAME = "&Quality";
	public static final String KEYWINDOW_MENUITEM_NAME = "&Key Window";
	public static final String ADVANCED_MENUITEM_NAME = "Ad&vanced";
	public static final String SCREENSHOT_MENUITEM_NAME = "&Screen Shot";
	public static final String HOSTKBD_MENUITEM_NAME = "&Host Keyboard";
	public static final String DIAGNOSIS_MENUITEM_NAME = "&Diagnosis";
	public static final String RAMDUMP_MENUITEM_NAME = "&Ram Dump";
	public static final String ABOUT_MENUITEM_NAME = "&About";
	public static final String FORCECLOSE_MENUITEM_NAME = "&Force Close";
	public static final String SDBSHELL_MENUITEM_NAME = "S&hell";
	public static final String ECP_MENUITEM_NAME = "Control &Panel";
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
	public MenuItem onTopItem;
	public MenuItem rotateItem;
	public MenuItem scaleItem;
	public MenuItem interpolationItem;
	public MenuItem interpolationHighItem;
	public MenuItem interpolationLowItem;
	public MenuItem keyWindowItem; /* key window menu */
	public MenuItem advancedItem; /* advanced menu */
	public MenuItem screenshotItem;
	public MenuItem hostKbdItem;
	public MenuItem hostKbdOnItem;
	public MenuItem hostKbdOffItem;
	public MenuItem diagnosisItem;
	public MenuItem ramdumpItem;
	public MenuItem aboutItem;
	public MenuItem forceCloseItem;
	public MenuItem shellItem;
	public MenuItem ecpItem;
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

		/* Emulator detail info menu */
		createDetailInfoItem(menu);

		new MenuItem(menu, SWT.SEPARATOR);

		/* Always on top menu */
		if (SwtUtil.isMacPlatform() == false) { /* not supported on mac */
			if (itemProperties == null || itemProperties.getTopmostItem() == null) {
				createOnTopItem(menu, TOPMOST_MENUITEM_NAME);
			} else {
				MenuItemType topmostMenuType = itemProperties.getTopmostItem();
				if (topmostMenuType.isVisible() == true) {
					createOnTopItem(menu, (topmostMenuType.getItemName().isEmpty()) ?
							TOPMOST_MENUITEM_NAME : topmostMenuType.getItemName());
				}
			}
		}

		/* Rotate menu */
		if (itemProperties == null || itemProperties.getRotateItem() == null) {
			createRotateItem(menu, ROTATE_MENUITEM_NAME);
		} else {
			MenuItemType rotationMenuType = itemProperties.getRotateItem();
			if (rotationMenuType.isVisible() == true) {
				createRotateItem(menu, (rotationMenuType.getItemName().isEmpty()) ?
						ROTATE_MENUITEM_NAME : rotationMenuType.getItemName());
			} else {
				skin.getEmulatorSkinState().setCurrentRotationId(
						RotationInfo.PORTRAIT.id());
			}
		}

		/* Scale menu */
		if (itemProperties == null || itemProperties.getScaleItem() == null) {
			createScaleItem(menu, SCALE_MENUITEM_NAME, null);
		} else {
			ScaleItemType scaleMenuType = itemProperties.getScaleItem();
			if (scaleMenuType.isVisible() == true) {
				String menuName = (scaleMenuType.getItemName().isEmpty()) ?
						SCALE_MENUITEM_NAME : scaleMenuType.getItemName();

				List<ScaleItemType.FactorItem> factors = scaleMenuType.getFactorItem();
				if (factors == null || factors.size() == 0) {
					logger.info("create a default Scale menu");

					createScaleItem(menu, menuName, null);
				} else {
					logger.info("create a custom Scale menu");

					createScaleItem(menu, menuName, factors);
				}
			} else {
				skin.getEmulatorSkinState().setCurrentScale(100);
			}
		}

		if (onTopItem != null || rotateItem != null || scaleItem != null) {
			new MenuItem(menu, SWT.SEPARATOR);
		}

		/* Key Window menu */
		if (itemProperties == null || itemProperties.getKeywindowItem() == null) {
			createKeyWindowItem(menu, KEYWINDOW_MENUITEM_NAME);
		} else {
			MenuItemType keywindowMenuType = itemProperties.getKeywindowItem();
			if (keywindowMenuType.isVisible() == true) {
				createKeyWindowItem(menu, (keywindowMenuType.getItemName().isEmpty()) ?
						KEYWINDOW_MENUITEM_NAME : keywindowMenuType.getItemName());
			}
		}

		/* Advanced menu */
		advancedItem = new MenuItem(menu, SWT.CASCADE);
		advancedItem.setText(ADVANCED_MENUITEM_NAME);
		advancedItem.setImage(imageRegistry.getIcon(IconName.ADVANCED));

		Menu advancedSubMenu = new Menu(menu.getShell(), SWT.DROP_DOWN);
		{
			/* Screen shot menu */
			createScreenShotItem(advancedSubMenu, SCREENSHOT_MENUITEM_NAME);

			/* VirtIO Keyboard menu */
			if (itemProperties == null || itemProperties.getHostKeyboardItem() == null) {
				createHostKbdItem(advancedSubMenu, HOSTKBD_MENUITEM_NAME);
			} else {
				MenuItemType hostKbdMenuType = itemProperties.getHostKeyboardItem();
				if (hostKbdMenuType.isVisible() == true) {
					createHostKbdItem(advancedSubMenu,
							(hostKbdMenuType.getItemName().isEmpty()) ?
							HOSTKBD_MENUITEM_NAME : hostKbdMenuType.getItemName());
				}
			}

			/* Diagnosis menu */
			if (SwtUtil.isLinuxPlatform()) { //TODO: windows
				diagnosisItem = new MenuItem(advancedSubMenu, SWT.CASCADE);
				diagnosisItem.setText(DIAGNOSIS_MENUITEM_NAME);
				diagnosisItem.setImage(imageRegistry.getIcon(IconName.DIAGNOSIS));

				Menu diagnosisSubMenu = new Menu(advancedSubMenu.getShell(), SWT.DROP_DOWN);
				{
					/* Ram Dump menu */
					createRamDumpItem(diagnosisSubMenu, RAMDUMP_MENUITEM_NAME);
				}
				diagnosisItem.setMenu(diagnosisSubMenu);
			}

			new MenuItem(advancedSubMenu, SWT.SEPARATOR);

			/* About menu */
			createAboutItem(advancedSubMenu, ABOUT_MENUITEM_NAME);

			new MenuItem(advancedSubMenu, SWT.SEPARATOR);

			/* Force close menu */
			createForceCloseItem(advancedSubMenu, FORCECLOSE_MENUITEM_NAME);
		}
		advancedItem.setMenu(advancedSubMenu);

		/* Shell menu */
		if (itemProperties == null || itemProperties.getShellItem() == null) {
			createShellItem(menu, SDBSHELL_MENUITEM_NAME);
		} else {
			MenuItemType shellMenuType = itemProperties.getShellItem();
			if (shellMenuType.isVisible() == true) {
				createShellItem(menu, (shellMenuType.getItemName().isEmpty()) ?
						SDBSHELL_MENUITEM_NAME : shellMenuType.getItemName());
			}
		}

		new MenuItem(menu, SWT.SEPARATOR);

		/* Emulator Control Panel menu */
		if (itemProperties == null || itemProperties.getControlPanelItem() == null) {
			createEcpItem(menu, ECP_MENUITEM_NAME);
		} else {
			MenuItemType ecpMenuType = itemProperties.getControlPanelItem();
			if (ecpMenuType.isVisible() == true) {
				createEcpItem(menu, (ecpMenuType.getItemName().isEmpty()) ?
						ECP_MENUITEM_NAME : ecpMenuType.getItemName());
			}
		}

		if (ecpItem != null) {
			new MenuItem(menu, SWT.SEPARATOR);
		}

		/* Close menu */
		createCloseItem(menu, CLOSE_MENUITEM_NAME);
	}

	private void createDetailInfoItem(Menu menu) {
		detailInfoItem = new MenuItem(menu, SWT.PUSH);
		detailInfoItem.setText(SkinUtil.makeEmulatorName(config));
		detailInfoItem.setImage(imageRegistry.getIcon(IconName.DETAIL_INFO));

		SelectionAdapter detailInfoListener = skin.createDetailInfoMenuListener();
		detailInfoItem.addSelectionListener(detailInfoListener);
	}

	private void createOnTopItem(Menu menu, String name) {
		onTopItem = new MenuItem(menu, SWT.CHECK);
		onTopItem.setText(name);
		onTopItem.setSelection(skin.isOnTop);

		SelectionAdapter topMostListener = skin.createTopMostMenuListener();
		onTopItem.addSelectionListener(topMostListener);
	}

	private void createRotateItem(Menu menu, String name) {
		rotateItem = new MenuItem(menu, SWT.CASCADE);
		rotateItem.setText(name);
		rotateItem.setImage(imageRegistry.getIcon(IconName.ROTATE));

		Menu rotateSubMenu = skin.createRotateMenu();
		rotateItem.setMenu(rotateSubMenu);
	}

	private void createScaleItem(Menu menu, String name,
			List<ScaleItemType.FactorItem> factors) {
		scaleItem = new MenuItem(menu, SWT.CASCADE);
		scaleItem.setText(name);
		scaleItem.setImage(imageRegistry.getIcon(IconName.SCALE));

		if (factors == null) {
			/* use default factor array */
			ScaleItemType.FactorItem actual = new ScaleItemType.FactorItem();
			actual.setItemName("1x");
			actual.setValue(100);

			ScaleItemType.FactorItem threeQuater = new ScaleItemType.FactorItem();
			threeQuater.setItemName("3/4x");
			threeQuater.setValue(75);

			ScaleItemType.FactorItem half = new ScaleItemType.FactorItem();
			half.setItemName("1/2x");
			half.setValue(50);

			ScaleItemType.FactorItem quater = new ScaleItemType.FactorItem();
			quater.setItemName("1/4x");
			quater.setValue(25);

			factors = Arrays.asList(actual, threeQuater, half, quater);
		}

		SelectionAdapter scaleListener = skin.createScaleMenuListener();

		Menu scaleSubMenu = new Menu(menu.getShell(), SWT.DROP_DOWN);
		{
			MenuItem matchedItem = null;

			for (ScaleItemType.FactorItem factor : factors) {
				final MenuItem menuItem = new MenuItem(scaleSubMenu, SWT.RADIO);
				menuItem.setText(factor.getItemName());
				menuItem.setData(factor.getValue());

				if (skin.getEmulatorSkinState().getCurrentScale()
						== (Integer) menuItem.getData()) {
					matchedItem = menuItem;
				}

				menuItem.addSelectionListener(scaleListener);
			}

			if (matchedItem == null) {
				matchedItem = scaleSubMenu.getItem(0);
				if (matchedItem == null) {
					return;
				}
				skin.getEmulatorSkinState().setCurrentScale(
						(Integer) matchedItem.getData());
			}

			matchedItem.setSelection(true);

			/* interpolation menu */
			createInterpolationItem(scaleSubMenu, INTERPOLATION_MENUITEM_NAME);
		}

		scaleItem.setMenu(scaleSubMenu);
	}

	private void createInterpolationItem(Menu menu, String name) {
		interpolationItem = new MenuItem(menu, SWT.CASCADE);
		interpolationItem.setText(name);

		Menu interpolationSubMenu = new Menu(menu.getShell(), SWT.DROP_DOWN);
		{
			createInterpolationHighLowItem(interpolationSubMenu);
		}
		interpolationItem.setMenu(interpolationSubMenu);
	}

	private void createInterpolationHighLowItem(Menu menu) {
		interpolationHighItem = new MenuItem(menu, SWT.RADIO);
		interpolationHighItem.setText("High");
		interpolationHighItem.setSelection(skin.isOnInterpolation);

		interpolationLowItem = new MenuItem(menu, SWT.RADIO);
		interpolationLowItem.setText("Low");
		interpolationLowItem.setSelection(!skin.isOnInterpolation);

		SelectionAdapter interpolationListener = skin.createInterpolationMenuListener();
		interpolationHighItem.addSelectionListener(interpolationListener);
		interpolationLowItem.addSelectionListener(interpolationListener);
	}

	private void createKeyWindowItem(Menu menu, String name) {
		/* load Key Window layout */
		SelectionAdapter keyWindowListener = skin.createKeyWindowMenuListener();

		String pathLayoutRoot = skin.skinInfo.getSkinPath() +
				File.separator + SpecialKeyWindow.KEYWINDOW_LAYOUT_ROOT;
		ArrayList<File> layouts = getKeyWindowLayoutList(pathLayoutRoot);

		if (layouts != null && layouts.size() != 0) {
			keyWindowItem = new MenuItem(menu, SWT.CASCADE);
			keyWindowItem.setText(name);

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

					keywindowLayoutItem.addSelectionListener(keyWindowListener);
				}
			}

			keyWindowItem.setMenu(keywindowSubMenu);
		} else { /* general key window */
			keyWindowItem = new MenuItem(menu, SWT.CHECK);
			keyWindowItem.setText(name);
			keyWindowItem.setSelection(skin.isKeyWindow);

			keyWindowItem.addSelectionListener(keyWindowListener);
		}
	}

	private void createScreenShotItem(Menu menu, String name) {
		screenshotItem = new MenuItem(menu, SWT.PUSH);
		screenshotItem.setText(name);
		screenshotItem.setImage(imageRegistry.getIcon(IconName.SCREENSHOT));

		SelectionAdapter screenshotListener = skin.createScreenshotMenuListener();
		screenshotItem.addSelectionListener(screenshotListener);
	}

	private void createHostKbdItem(Menu menu, String name) {
		hostKbdItem = new MenuItem(menu, SWT.CASCADE);
		hostKbdItem.setText(name);
		hostKbdItem.setImage(imageRegistry.getIcon(IconName.HOST_KBD));

		Menu hostKbdSubMenu = new Menu(menu.getShell(), SWT.DROP_DOWN);
		{
			createKbdOnOffItem(hostKbdSubMenu);
		}
		hostKbdItem.setMenu(hostKbdSubMenu);
	}

	private void createKbdOnOffItem(Menu menu) {
		hostKbdOnItem = new MenuItem(menu, SWT.RADIO);
		hostKbdOnItem.setText("On");
		hostKbdOnItem.setSelection(skin.isOnKbd);

		hostKbdOffItem = new MenuItem(menu, SWT.RADIO);
		hostKbdOffItem.setText("Off");
		hostKbdOffItem.setSelection(!skin.isOnKbd);

		SelectionAdapter hostKbdListener = skin.createHostKbdMenuListener();
		hostKbdOnItem.addSelectionListener(hostKbdListener);
		hostKbdOffItem.addSelectionListener(hostKbdListener);
	}

	private void createRamDumpItem(Menu menu, String name) {
		ramdumpItem = new MenuItem(menu, SWT.PUSH);
		ramdumpItem.setText(name);

		SelectionAdapter ramdumpListener = skin.createRamdumpMenuListener();
		ramdumpItem.addSelectionListener(ramdumpListener);
	}

	private void createAboutItem(Menu menu, String name) {
		aboutItem = new MenuItem(menu, SWT.PUSH);
		aboutItem.setText(name);
		aboutItem.setImage(imageRegistry.getIcon(IconName.ABOUT));

		SelectionAdapter aboutListener = skin.createAboutMenuListener();
		aboutItem.addSelectionListener(aboutListener);
	}

	private void createForceCloseItem(Menu menu, String name) {
		forceCloseItem = new MenuItem(menu, SWT.PUSH);
		forceCloseItem.setText(name);
		forceCloseItem.setImage(imageRegistry.getIcon(IconName.FORCE_CLOSE));

		SelectionAdapter forceCloseListener = skin.createForceCloseMenuListener();
		forceCloseItem.addSelectionListener(forceCloseListener);
	}

	private void createShellItem(Menu menu, String name) {
		shellItem = new MenuItem(menu, SWT.PUSH);
		shellItem.setText(name);
		shellItem.setImage(imageRegistry.getIcon(IconName.SHELL));

		SelectionAdapter shellListener = skin.createShellMenuListener();
		shellItem.addSelectionListener(shellListener);
	}

	private void createEcpItem(Menu menu, String name) {
		ecpItem = new MenuItem(menu, SWT.PUSH);
		ecpItem.setText(name);
		ecpItem.setImage(imageRegistry.getIcon(IconName.ECP));

		SelectionAdapter ecpListener = skin.createEcpMenuListener();
		ecpItem.addSelectionListener(ecpListener);
	}

	private void createCloseItem(Menu menu, String name) {
		closeItem = new MenuItem(menu, SWT.PUSH);
		closeItem.setText(name);
		closeItem.setImage(imageRegistry.getIcon(IconName.CLOSE));

		SelectionAdapter closeListener = skin.createCloseMenuListener();
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
