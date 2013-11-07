/**
 * Special Key Window
 *
 * Copyright (C) 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * GiWoong Kim <giwoong.kim@samsung.com>
 * Hyunjin Lee <hyunjin816.lee@samsung.com>
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

package org.tizen.emulator.skin.custom;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.util.List;
import java.util.logging.Level;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.events.MouseMoveListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.events.ShellEvent;
import org.eclipse.swt.events.ShellListener;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.graphics.Region;
import org.eclipse.swt.widgets.Shell;
import org.tizen.emulator.skin.EmulatorSkin;
import org.tizen.emulator.skin.comm.ICommunicator.KeyEventType;
import org.tizen.emulator.skin.comm.ICommunicator.MouseButtonType;
import org.tizen.emulator.skin.comm.ICommunicator.MouseEventType;
import org.tizen.emulator.skin.comm.ICommunicator.SendCommand;
import org.tizen.emulator.skin.comm.sock.SocketCommunicator;
import org.tizen.emulator.skin.comm.sock.data.KeyEventData;
import org.tizen.emulator.skin.comm.sock.data.MouseEventData;
import org.tizen.emulator.skin.config.EmulatorConfig;
import org.tizen.emulator.skin.exception.JaxbException;
import org.tizen.emulator.skin.image.SpecialKeyWindowImageRegistry;
import org.tizen.emulator.skin.image.SpecialKeyWindowImageRegistry.SpecailKeyWindowImageType;
import org.tizen.emulator.skin.keywindow.dbi.EventInfoType;
import org.tizen.emulator.skin.keywindow.dbi.KeyMapType;
import org.tizen.emulator.skin.keywindow.dbi.KeyWindowUI;
import org.tizen.emulator.skin.keywindow.dbi.RegionType;
import org.tizen.emulator.skin.layout.HWKey;
import org.tizen.emulator.skin.util.IOUtil;
import org.tizen.emulator.skin.util.JaxbUtil;
import org.tizen.emulator.skin.util.HWKeyRegion;
import org.tizen.emulator.skin.util.SkinUtil;
import org.tizen.emulator.skin.util.SwtUtil;

public class SpecialKeyWindow extends SkinWindow {
	public static final String KEYWINDOW_LAYOUT_ROOT = "keywindow-layout";
	public static final String DBI_FILE_NAME = "default.dbi";

	private EmulatorSkin skin;
	private SpecialKeyWindowImageRegistry imageRegistry;
	private SocketCommunicator communicator;

	private KeyWindowUI dbiContents;
	private Image keyWindowImage;
	private Image keyWindowPressedImage;

	private ShellListener shellListener;
	private PaintListener shellPaintListener;
	private MouseMoveListener shellMouseMoveListener;
	private MouseListener shellMouseListener;

	private boolean isGrabbedShell;
	private Point grabPosition;
	private HWKey currentPressedHWKey;
	private boolean isTouch;

	public SpecialKeyWindow(EmulatorSkin skin, String layoutName) {
		super(skin.getShell(), SWT.RIGHT | SWT.CENTER);

		this.skin = skin;
		this.parent = skin.getShell();
		this.shell = new Shell(parent.getDisplay() /* for Mac & Always on Top */,
				SWT.NO_TRIM | SWT.RESIZE | SWT.TOOL | SWT.NO_FOCUS);
		this.communicator = skin.communicator;

		this.isGrabbedShell= false;
		this.grabPosition = new Point(0, 0);

		shell.setText(parent.getText());
		shell.setBackground(parent.getBackground());
		shell.setImage(parent.getImage());

		/* load dbi file */
		String skinPath = skin.skinInfo.getSkinPath();
		String specialKeyWindowPath = skinPath + File.separator
				+ KEYWINDOW_LAYOUT_ROOT + File.separator + layoutName;
		logger.info("special key window path : " + specialKeyWindowPath);

		this.dbiContents = loadXMLForKeyWindow(specialKeyWindowPath);

		/* image init */
		this.imageRegistry = new SpecialKeyWindowImageRegistry(
				shell.getDisplay(), dbiContents, specialKeyWindowPath);

		/* get keywindow image */
		//TODO: null
		this.keyWindowImage = imageRegistry.getKeyWindowImage(
				EmulatorConfig.DEFAULT_WINDOW_ROTATION,
				SpecailKeyWindowImageType.SPECIAL_IMAGE_TYPE_NORMAL);
		this.keyWindowPressedImage = imageRegistry.getKeyWindowImage(
				EmulatorConfig.DEFAULT_WINDOW_ROTATION,
				SpecailKeyWindowImageType.SPECIAL_IMAGE_TYPE_PRESSED);

		/* set window size */
		if (keyWindowImage != null) {
			ImageData imageData = keyWindowImage.getImageData();
			shell.setSize(imageData.width, imageData.height);
		}

		/* custom window shape */
		Region region = SkinUtil.getTrimmingRegion(keyWindowImage);
		if (region != null) {
			shell.setRegion(region);
		}

		addKeyWindowListener();
	}

	private KeyWindowUI loadXMLForKeyWindow(String skinPath) {
		String dbiPath = skinPath + File.separator + DBI_FILE_NAME;
		logger.info("load dbi file from " + dbiPath);

		FileInputStream fis = null;
		KeyWindowUI keyWindowUI = null;

		try {
			fis = new FileInputStream(dbiPath);
			logger.info("============ dbi contents ============");
			byte[] bytes = IOUtil.getBytes(fis);
			logger.info(new String(bytes, "UTF-8"));
			logger.info("=======================================");

			keyWindowUI = JaxbUtil.unmarshal(bytes, KeyWindowUI.class);
		} catch (IOException e) {
			logger.log(Level.SEVERE, e.getMessage(), e);
		} catch (JaxbException e) {
			logger.log(Level.SEVERE, e.getMessage(), e);
		} finally {
			IOUtil.close(fis);
		}

		return keyWindowUI;
	}

	private HWKey getHWKey(int currentX, int currentY) {
		List<KeyMapType> keyMapList = dbiContents.getKeyMapList().getKeyMap();
		if (keyMapList == null) {
			return null;
		}

		for (KeyMapType keyEntry : keyMapList) {
			RegionType region = keyEntry.getRegion();

			int scaledX = (int) region.getLeft();
			int scaledY = (int) region.getTop();
			int scaledWidth = (int) region.getWidth();
			int scaledHeight = (int) region.getHeight();

			if (SkinUtil.isInGeometry(currentX, currentY,
					scaledX, scaledY, scaledWidth, scaledHeight)) {
				EventInfoType eventInfo = keyEntry.getEventInfo();

				HWKey hwKey = new HWKey(
						eventInfo.getKeyName(),
						eventInfo.getKeyCode(),
						new HWKeyRegion(scaledX, scaledY, scaledWidth, scaledHeight),
						keyEntry.getTooltip());

				return hwKey;
			}
		}

		return null;
	}

	private void addKeyWindowListener() {
		shellPaintListener = new PaintListener() {
			@Override
			public void paintControl(final PaintEvent e) {
				if (keyWindowImage != null) {
					e.gc.drawImage(keyWindowImage, 0, 0);
				}				
			}
		};

		shell.addPaintListener(shellPaintListener);

		shellListener = new ShellListener() {
			@Override
			public void shellClosed(ShellEvent event) {
				logger.info("Special Key Window is closed");

				dispose();
			}

			@Override
			public void shellActivated(ShellEvent event) {
				logger.info("activate");

				if (SwtUtil.isMacPlatform() == true) {
					parent.moveAbove(shell);
				} else {
					shell.getDisplay().asyncExec(new Runnable() {
						@Override
						public void run() {
							parent.setActive();
						}
					});
				}
			}

			@Override
			public void shellDeactivated(ShellEvent event) {
				logger.info("deactivate");

				/* do nothing */
			}

			@Override
			public void shellIconified(ShellEvent event) {
				/* do nothing */
			}

			@Override
			public void shellDeiconified(ShellEvent event) {
				logger.info("deiconified");

				shell.getDisplay().asyncExec(new Runnable() {
					@Override
					public void run() {
						if (parent.getMinimized() == true) {
							parent.setMinimized(false);
						}
					}
				});
			}
		};

		shell.addShellListener(shellListener);

		shellMouseMoveListener = new MouseMoveListener() {
			@Override
			public void mouseMove(MouseEvent e) {
				if (isTouch == true) {
					logger.info("mouseMove in KeyWindow : " + e.x + ", " + e.y);

					HWKey pressedHWKey = currentPressedHWKey;
					int x = pressedHWKey.getRegion().x;
					int y = pressedHWKey.getRegion().y;
					int width = pressedHWKey.getRegion().width;
					int height = pressedHWKey.getRegion().height;
					//int eventType;

					if (SkinUtil.isInGeometry(e.x, e.y, x, y, width, height)) {
						//eventType = MouseEventType.DRAG.value();
					} else {
						isTouch = false;
						//eventType = MouseEventType.RELEASE.value();
						/* rollback a keyPressed image region */
						shell.redraw(x, y, width, height, false);
					}

					MouseEventData mouseEventData = new MouseEventData(
							MouseButtonType.LEFT.value(), MouseEventType.DRAG.value(),
							e.x, e.y, e.x, e.y, 0);
					communicator.sendToQEMU(
							SendCommand.SEND_MOUSE_EVENT, mouseEventData, false);

					return;
				} else if (isGrabbedShell == true && e.button == 0/* left button */) {
					if (getDockPosition() != SWT.NONE) {
						dock(SWT.NONE, false, false);
						shell.moveAbove(parent);
					}

					/* move a window */
					Point previousLocation = shell.getLocation();
					int x = previousLocation.x + (e.x - grabPosition.x);
					int y = previousLocation.y + (e.y - grabPosition.y);

					shell.setLocation(x, y);

					return;
				}
			}
		};

		shell.addMouseMoveListener(shellMouseMoveListener);

		shellMouseListener = new MouseListener() {
			@Override
			public void mouseUp(MouseEvent e) {
				if (e.button == 1) { /* left button */
					isGrabbedShell = false;
					grabPosition.x = grabPosition.y = 0;

					/* HW key handling */
					HWKey pressedHWKey = currentPressedHWKey;
					if (pressedHWKey == null) {
						logger.info("mouseUp in KeyWindow : " + e.x + ", " + e.y);

						/* Let me check whether the key window was landed
						 * on docking area. */
						Rectangle parentBounds = parent.getBounds();
						Rectangle childBounds = shell.getBounds();

						int heightOneThird = parentBounds.height / 3;
						int widthDockingArea = 30;
						int widthIntersectRegion = 5;

						/* right-middle */
						Rectangle attachBoundsRC = new Rectangle(
								(parentBounds.x + parentBounds.width) - widthIntersectRegion,
								parentBounds.y + heightOneThird,
								widthDockingArea, heightOneThird);
						/* right-top */
						Rectangle attachBoundsRT = new Rectangle(
								(parentBounds.x + parentBounds.width) - widthIntersectRegion,
								parentBounds.y,
								widthDockingArea, heightOneThird);
						/* right-bottom */
						Rectangle attachBoundsRB = new Rectangle(
								(parentBounds.x + parentBounds.width) - widthIntersectRegion,
								parentBounds.y + (heightOneThird * 2),
								widthDockingArea, heightOneThird);

						/* left-middle */
						Rectangle attachBoundsLC = new Rectangle(
								parentBounds.x - (widthDockingArea - widthIntersectRegion),
								parentBounds.y + heightOneThird,
								widthDockingArea, heightOneThird);
						/* left-top */
						Rectangle attachBoundsLT = new Rectangle(
								parentBounds.x - (widthDockingArea - widthIntersectRegion),
								parentBounds.y,
								widthDockingArea, heightOneThird);
						/* left-bottom */
						Rectangle attachBoundsLB = new Rectangle(
								parentBounds.x - (widthDockingArea - widthIntersectRegion),
								parentBounds.y + (heightOneThird * 2),
								widthDockingArea, heightOneThird);

						if (childBounds.intersects(attachBoundsRC) == true) {
							dock(SWT.RIGHT | SWT.CENTER, false, true);
						} else if (childBounds.intersects(attachBoundsRT) == true) {
							dock(SWT.RIGHT | SWT.TOP, false, true);
						} else if (childBounds.intersects(attachBoundsRB) == true) {
							dock(SWT.RIGHT | SWT.BOTTOM, false, true);
						} else if (childBounds.intersects(attachBoundsLC) == true) {
							dock(SWT.LEFT | SWT.CENTER, false, true);
						} else if (childBounds.intersects(attachBoundsLT) == true) {
							dock(SWT.LEFT | SWT.TOP, false, true);
						} else if (childBounds.intersects(attachBoundsLB) == true) {
							dock(SWT.LEFT | SWT.BOTTOM, false, true);
						} else {
							dock(SWT.NONE, false, true);
						}

						return;
					}

					if (pressedHWKey.getKeyCode() != SkinUtil.UNKNOWN_KEYCODE) {
						logger.info(pressedHWKey.getName() + " key is released");

						/* send event */
						if (isTouch) {
							isTouch = false;
							MouseEventData mouseEventData = new MouseEventData(
									MouseButtonType.LEFT.value(), MouseEventType.RELEASE.value(),
									e.x, e.y, e.x, e.y, 0);
							communicator.sendToQEMU(
									SendCommand.SEND_MOUSE_EVENT, mouseEventData, false);
						} else {
							KeyEventData keyEventData = new KeyEventData(
									KeyEventType.RELEASED.value(), pressedHWKey.getKeyCode(), 0, 0);
							communicator.sendToQEMU(
									SendCommand.SEND_HW_KEY_EVENT, keyEventData, false);
						}

						currentPressedHWKey = null;

						/* rollback a keyPressed image resion */
						shell.redraw(pressedHWKey.getRegion().x, pressedHWKey.getRegion().y, 
								pressedHWKey.getRegion().width, pressedHWKey.getRegion().height, false);
					}						
				}
			}

			@Override
			public void mouseDown(MouseEvent e) {
				if (1 == e.button) { /* left button */
					/* HW key handling */
					final HWKey hwKey = getHWKey(e.x, e.y);
					if (hwKey == null) {
						logger.info("mouseDown in KeyWindow : " + e.x + ", " + e.y);

						isGrabbedShell = true;
						grabPosition.x = e.x;
						grabPosition.y = e.y;

						return;						
					}

					if (hwKey.getKeyCode() != SkinUtil.UNKNOWN_KEYCODE) {
						logger.info(hwKey.getName() + " key is pressed");

						/* send event */
						if (hwKey.getTooltip().equalsIgnoreCase("touch")) {
							isTouch = true;
							MouseEventData mouseEventData = new MouseEventData(
									MouseButtonType.LEFT.value(), MouseEventType.PRESS.value(),
									e.x, e.y, e.x, e.y, 0);
							communicator.sendToQEMU(
									SendCommand.SEND_MOUSE_EVENT, mouseEventData, false);
						} else {
							KeyEventData keyEventData = new KeyEventData(
									KeyEventType.PRESSED.value(), hwKey.getKeyCode(), 0, 0);
							communicator.sendToQEMU(
									SendCommand.SEND_HW_KEY_EVENT, keyEventData, false);
						}

						currentPressedHWKey = hwKey;

						shell.setToolTipText(null);

						/* draw the HW key region as the cropped keyPressed image area */
						if (hwKey.getRegion() != null &&
								hwKey.getRegion().width != 0 && hwKey.getRegion().height != 0) {
							shell.getDisplay().syncExec(new Runnable() {
								public void run() {
									if (keyWindowPressedImage != null) {
										GC gc = new GC(shell);
										if (gc != null) {
											gc.drawImage(keyWindowPressedImage, 
													hwKey.getRegion().x, hwKey.getRegion().y, 
													hwKey.getRegion().width, hwKey.getRegion().height, 
													hwKey.getRegion().x, hwKey.getRegion().y, 
													hwKey.getRegion().width, hwKey.getRegion().height);
											gc.dispose();
										}										
									}
								} /* end of run */
							});
						}

					}
				}
			}

			@Override
			public void mouseDoubleClick(MouseEvent e) {
				/* do nothing */
			}
		};

		shell.addMouseListener(shellMouseListener);
	}

	private void dispose() {
		if (skin.pairTag != null) {
			skin.pairTag.setVisible(false);
		}

		if (null != shellPaintListener) {
			shell.removePaintListener(shellPaintListener);
		}

		if (null != shellListener) {
			shell.removeShellListener(shellListener);
		}

		if (null != shellMouseMoveListener) {
			shell.removeMouseMoveListener(shellMouseMoveListener);
		}

		if (null != shellMouseListener) {
			shell.removeMouseListener(shellMouseListener);
		}

		imageRegistry.dispose();
	}
}
