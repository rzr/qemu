/**
 * Right Click Popup Menu
 *
 * Copyright (C) 2013 - 2014 Samsung Electronics Co., Ltd. All rights reserved.
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
import org.eclipse.swt.graphics.Image;
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
		detailInfoItem = createDetailInfoItem(menu);

		if (detailInfoItem != null) {
			new MenuItem(menu, SWT.SEPARATOR);
		}

		/* Always on top menu */
		if (SwtUtil.isMacPlatform() == false) { /* not supported on mac */
			if (itemProperties == null ||
					itemProperties.getTopmostItem() == null) {
				onTopItem = createOnTopItem(menu, TOPMOST_MENUITEM_NAME);
			} else {
				MenuItemType topmostMenuType = itemProperties.getTopmostItem();
				if (topmostMenuType.isVisible() == true) {
					onTopItem = createOnTopItem(menu,
							(topmostMenuType.getItemName().isEmpty()) ?
							TOPMOST_MENUITEM_NAME : topmostMenuType.getItemName());
				}
			}
		}

		/* Rotate menu */
		if (itemProperties == null ||
				itemProperties.getRotateItem() == null) {
			rotateItem = createRotateItem(menu, ROTATE_MENUITEM_NAME);
		} else {
			MenuItemType rotationMenuType = itemProperties.getRotateItem();
			if (rotationMenuType.isVisible() == true) {
				rotateItem = createRotateItem(menu,
						(rotationMenuType.getItemName().isEmpty()) ?
						ROTATE_MENUITEM_NAME : rotationMenuType.getItemName());
			} else {
				/* do not rotate */
				skin.getEmulatorSkinState().setCurrentRotationId(
						RotationInfo.PORTRAIT.id());
			}
		}

		/* Scale menu */
		if (itemProperties == null ||
				itemProperties.getScaleItem() == null) {
			scaleItem = createScaleItem(menu, SCALE_MENUITEM_NAME, null);
		} else {
			ScaleItemType scaleMenuType = itemProperties.getScaleItem();
			if (scaleMenuType.isVisible() == true) {
				String menuName = (scaleMenuType.getItemName().isEmpty()) ?
						SCALE_MENUITEM_NAME : scaleMenuType.getItemName();

				List<ScaleItemType.FactorItem> factors = scaleMenuType.getFactorItem();
				if (factors == null || factors.size() == 0) {
					logger.info("create a predefined Scale menu");

					scaleItem = createScaleItem(menu, menuName, null);
				} else {
					logger.info("create a custom Scale menu");

					scaleItem = createScaleItem(menu, menuName, factors);
				}
			} else {
				/* do not scaling */
				skin.getEmulatorSkinState().setCurrentScale(100);
			}
		}

		if (onTopItem != null || rotateItem != null || scaleItem != null) {
			new MenuItem(menu, SWT.SEPARATOR);
		}

		/* Key Window menu */
		if (itemProperties == null ||
				itemProperties.getKeywindowItem() == null) {
			keyWindowItem = createKeyWindowItem(menu, KEYWINDOW_MENUITEM_NAME);
		} else {
			MenuItemType keywindowMenuType = itemProperties.getKeywindowItem();
			if (keywindowMenuType.isVisible() == true) {
				keyWindowItem = createKeyWindowItem(menu,
						(keywindowMenuType.getItemName().isEmpty()) ?
						KEYWINDOW_MENUITEM_NAME : keywindowMenuType.getItemName());
			}
		}

		/* Advanced menu */
		Menu advancedSubMenu = new Menu(menu.getShell(), SWT.DROP_DOWN);
		{
			/* Screen shot menu */
			screenshotItem = createScreenShotItem(
					advancedSubMenu, SCREENSHOT_MENUITEM_NAME);

			/* VirtIO Keyboard menu */
			if (itemProperties == null ||
					itemProperties.getHostKeyboardItem() == null) {
				hostKbdItem = createHostKbdItem(advancedSubMenu, HOSTKBD_MENUITEM_NAME);
			} else {
				MenuItemType hostKbdMenuType = itemProperties.getHostKeyboardItem();
				if (hostKbdMenuType.isVisible() == true) {
					hostKbdItem = createHostKbdItem(advancedSubMenu,
							(hostKbdMenuType.getItemName().isEmpty()) ?
							HOSTKBD_MENUITEM_NAME : hostKbdMenuType.getItemName());
				}
			}

			/* Diagnosis menu */
			if (SwtUtil.isLinuxPlatform()) { //TODO: windows
				Menu diagnosisSubMenu = new Menu(advancedSubMenu.getShell(), SWT.DROP_DOWN);
				{
					/* Ram Dump menu */
					if (itemProperties == null ||
							itemProperties.getRamdumpItem() == null) {
						ramdumpItem = createRamDumpItem(diagnosisSubMenu, RAMDUMP_MENUITEM_NAME);
					} else {
						MenuItemType ramdumpMenuType = itemProperties.getRamdumpItem();
						if (ramdumpMenuType.isVisible() == true) {
							ramdumpItem = createRamDumpItem(diagnosisSubMenu,
									(ramdumpMenuType.getItemName().isEmpty()) ?
									RAMDUMP_MENUITEM_NAME : ramdumpMenuType.getItemName());
						}
					}
				}

				if (ramdumpItem != null) {
					diagnosisItem = new MenuItem(advancedSubMenu, SWT.CASCADE);
					diagnosisItem.setText(DIAGNOSIS_MENUITEM_NAME);
					diagnosisItem.setImage(imageRegistry.getIcon(IconName.DIAGNOSIS));
					diagnosisItem.setMenu(diagnosisSubMenu);
				}
			}

			if (screenshotItem != null || hostKbdItem != null || diagnosisItem != null) {
				advancedItem = new MenuItem(menu, SWT.CASCADE);
				advancedItem.setText(ADVANCED_MENUITEM_NAME);
				advancedItem.setImage(imageRegistry.getIcon(IconName.ADVANCED));

				new MenuItem(advancedSubMenu, SWT.SEPARATOR);
			}

			/* About menu */
			aboutItem = createAboutItem(
					advancedSubMenu, ABOUT_MENUITEM_NAME);

			if (aboutItem != null) {
				new MenuItem(advancedSubMenu, SWT.SEPARATOR);
			}

			/* Force close menu */
			forceCloseItem = createForceCloseItem(
					advancedSubMenu, FORCECLOSE_MENUITEM_NAME);
		}
		advancedItem.setMenu(advancedSubMenu);

		/* Shell menu */
		if (itemProperties == null ||
				itemProperties.getShellItem() == null) {
			shellItem = createShellItem(menu, SDBSHELL_MENUITEM_NAME);
		} else {
			MenuItemType shellMenuType = itemProperties.getShellItem();
			if (shellMenuType.isVisible() == true) {
				shellItem = createShellItem(menu,
						(shellMenuType.getItemName().isEmpty()) ?
						SDBSHELL_MENUITEM_NAME : shellMenuType.getItemName());
			}
		}

		if (keyWindowItem != null || advancedItem != null || shellItem != null) {
			new MenuItem(menu, SWT.SEPARATOR);
		}

		/* Emulator Control Panel menu */
		if (itemProperties == null ||
				itemProperties.getControlPanelItem() == null) {
			ecpItem = createEcpItem(menu, ECP_MENUITEM_NAME);
		} else {
			MenuItemType ecpMenuType = itemProperties.getControlPanelItem();
			if (ecpMenuType.isVisible() == true) {
				ecpItem = createEcpItem(menu,
						(ecpMenuType.getItemName().isEmpty()) ?
						ECP_MENUITEM_NAME : ecpMenuType.getItemName());
			}
		}

		if (ecpItem != null) {
			new MenuItem(menu, SWT.SEPARATOR);
		}

		/* Close menu */
		closeItem = createCloseItem(menu, CLOSE_MENUITEM_NAME);
	}

	private MenuItem createMenuItem(Menu menu, int style,
			String name, Image image, SelectionAdapter listener, boolean selected) {
		MenuItem item = new MenuItem(menu, style);

		if (name != null) {
			item.setText(name);
		}

		if (image != null) {
			item.setImage(image);
		}

		if (listener != null) {
			item.addSelectionListener(listener);
		}

		item.setSelection(selected);

		return item;
	}

	private MenuItem createDetailInfoItem(Menu menu) {
		return createMenuItem(menu, SWT.PUSH,
				SkinUtil.makeEmulatorName(config),
				imageRegistry.getIcon(IconName.DETAIL_INFO),
				skin.createDetailInfoMenuListener(),
				false);
	}

	private MenuItem createOnTopItem(Menu menu, String name) {
		return createMenuItem(menu, SWT.CHECK,
				name,
				null,
				skin.createTopMostMenuListener(),
				skin.isOnTop);
	}

	private MenuItem createRotateItem(Menu menu, String name) {
		MenuItem item = createMenuItem(menu, SWT.CASCADE,
				name,
				imageRegistry.getIcon(IconName.ROTATE),
				null,
				false);

		item.setMenu(skin.createRotateMenu());

		return item;
	}

	private MenuItem createScaleItem(Menu menu, String name,
			List<ScaleItemType.FactorItem> factors) {
		MenuItem item = createMenuItem(menu, SWT.CASCADE,
				name,
				imageRegistry.getIcon(IconName.SCALE),
				null,
				false);

		item.setMenu(createScaleSubItem(menu, factors));

		return item;
	}

	private Menu createScaleSubItem(Menu menu,
			List<ScaleItemType.FactorItem> factors) {
		SelectionAdapter scaleListener =
				skin.createScaleMenuListener();

		Menu subMenu = new Menu(menu.getShell(), SWT.DROP_DOWN);

		if (factors == null) {
			/* use predefined factor array */
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

		MenuItem matchedItem = null;
		for (ScaleItemType.FactorItem factor : factors) {
			String factorName = factor.getItemName();
			if (factorName == null || factorName.isEmpty()) {
				factorName = factor.getValue() + "%";
			}

			final MenuItem factorItem = createMenuItem(subMenu, SWT.RADIO,
					factorName,
					null,
					scaleListener,
					false);

			factorItem.setData(factor.getValue());

			if (skin.getEmulatorSkinState().getCurrentScale()
					== (Integer) factorItem.getData()) {
				matchedItem = factorItem;
			}
		}

		if (matchedItem == null) {
			matchedItem = subMenu.getItem(0);
			if (matchedItem == null) {
				return null;
			}

			skin.getEmulatorSkinState().setCurrentScale(
					(Integer) matchedItem.getData());
		}

		matchedItem.setSelection(true);

		/* interpolation menu */
		interpolationItem = createInterpolationItem(
				subMenu, INTERPOLATION_MENUITEM_NAME);

		return subMenu;
	}

	private MenuItem createInterpolationItem(Menu menu, String name) {
		MenuItem item = createMenuItem(menu, SWT.CASCADE,
				name, null, null, false);

		item.setMenu(createInterpolationSubItem(menu));

		return item;
	}

	private Menu createInterpolationSubItem(Menu menu) {
		SelectionAdapter interpolationListener =
				skin.createInterpolationMenuListener();

		Menu subMenu = new Menu(menu.getShell(), SWT.DROP_DOWN);

		interpolationHighItem = createMenuItem(subMenu, SWT.RADIO,
				"High", null,
				interpolationListener,
				skin.isOnInterpolation);

		interpolationLowItem = createMenuItem(subMenu, SWT.RADIO,
				"Low", null,
				interpolationListener,
				!skin.isOnInterpolation);

		return subMenu;
	}

	private MenuItem createKeyWindowItem(Menu menu, String name) {
		/* load Key Window layouts */
		String pathLayoutRoot = skin.skinInfo.getSkinPath() +
				File.separator + SpecialKeyWindow.KEYWINDOW_LAYOUT_ROOT;
		ArrayList<File> layouts = getKeyWindowLayoutList(pathLayoutRoot);

		MenuItem item = null;
		if (layouts != null && layouts.size() != 0) {
			item = createMenuItem(menu, SWT.CASCADE,
					name, null, null, false);

			item.setMenu(createKeyWindowSubItem(menu, layouts));
		} else { /* no need to create sub menu for general key window */
			item = createMenuItem(menu, SWT.CHECK,
					name,
					null,
					skin.createKeyWindowMenuListener(),
					skin.isKeyWindow);
		}

		return item;
	}

	private Menu createKeyWindowSubItem(Menu menu,
			ArrayList<File> layouts) {
		SelectionAdapter keyWindowListener =
				skin.createKeyWindowMenuListener();

		Menu subMenu = new Menu(menu.getShell(), SWT.DROP_DOWN);

		for (int i = 0; i < layouts.size(); i++) {
			File dir = layouts.get(i);

			createMenuItem(subMenu, SWT.CHECK,
					dir.getName(),
					null,
					keyWindowListener,
					(i == 0) ? true : false);
		}

		return subMenu;
	}

	private MenuItem createScreenShotItem(Menu menu, String name) {
		return createMenuItem(menu, SWT.PUSH,
				name,
				imageRegistry.getIcon(IconName.SCREENSHOT),
				skin.createScreenshotMenuListener(),
				false);
	}

	private MenuItem createHostKbdItem(Menu menu, String name) {
		MenuItem item = createMenuItem(menu, SWT.CASCADE,
				name,
				imageRegistry.getIcon(IconName.HOST_KBD),
				null,
				false);

		item.setMenu(createKbdSubItem(menu));

		return item;
	}

	private Menu createKbdSubItem(Menu menu) {
		SelectionAdapter hostKbdListener =
				skin.createHostKbdMenuListener();

		Menu subMenu = new Menu(menu.getShell(), SWT.DROP_DOWN);

		hostKbdOnItem = createMenuItem(subMenu, SWT.RADIO,
				"On", null,
				hostKbdListener,
				skin.isOnKbd);

		hostKbdOffItem = createMenuItem(subMenu, SWT.RADIO,
				"Off", null,
				hostKbdListener,
				!skin.isOnKbd);

		return subMenu;
	}

	private MenuItem createRamDumpItem(Menu menu, String name) {
		return createMenuItem(menu, SWT.PUSH,
				name,
				null,
				skin.createRamdumpMenuListener(),
				false);
	}

	private MenuItem createAboutItem(Menu menu, String name) {
		return createMenuItem(menu, SWT.PUSH,
				name,
				imageRegistry.getIcon(IconName.ABOUT),
				skin.createAboutMenuListener(),
				false);
	}

	private MenuItem createForceCloseItem(Menu menu, String name) {
		return createMenuItem(menu, SWT.PUSH,
				name,
				imageRegistry.getIcon(IconName.FORCE_CLOSE),
				skin.createForceCloseMenuListener(),
				false);
	}

	private MenuItem createShellItem(Menu menu, String name) {
		return createMenuItem(menu, SWT.PUSH,
				name,
				imageRegistry.getIcon(IconName.SHELL),
				skin.createShellMenuListener(),
				false);
	}

	private MenuItem createEcpItem(Menu menu, String name) {
		return createMenuItem(menu, SWT.PUSH,
				name,
				imageRegistry.getIcon(IconName.ECP),
				skin.createEcpMenuListener(),
				false);
	}

	private MenuItem createCloseItem(Menu menu, String name) {
		return createMenuItem(menu, SWT.PUSH,
				name,
				imageRegistry.getIcon(IconName.CLOSE),
				skin.createCloseMenuListener(),
				false);
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
			if (layoutPaths == null || layoutPaths.length <= 0) {
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
