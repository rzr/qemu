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
import java.util.logging.Level;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.events.MouseMoveListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.events.ShellEvent;
import org.eclipse.swt.events.ShellListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.RGB;
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
import org.tizen.emulator.skin.dbi.EmulatorUI;
import org.tizen.emulator.skin.exception.JaxbException;
import org.tizen.emulator.skin.image.SpecialKeyWindowImageRegistry;
import org.tizen.emulator.skin.image.SpecialKeyWindowImageRegistry.SpecailKeyWindowImageType;
import org.tizen.emulator.skin.layout.HWKey;
import org.tizen.emulator.skin.util.IOUtil;
import org.tizen.emulator.skin.util.JaxbUtil;
import org.tizen.emulator.skin.util.SpecialKeyWindowUtil;
import org.tizen.emulator.skin.util.SwtUtil;

public class SpecialKeyWindow extends SkinWindow {
	private static final String KEYWINDOW_LAYOUT = "keywindow-layout";	
	private static final String DBI_FILE_NAME = "default.dbi";	

	private EmulatorSkin skin;
	
	private HWKey currentPressedHWKey;

	private int widthBase;
	private int heightBase;

	private Image keyWindowImage;
	private Image keyWindowPressedImage;

	private Color colorFrame;
	private SpecialKeyWindowImageRegistry imageRegistry;
	private SocketCommunicator communicator;	

	private ShellListener shellListener;
	private PaintListener shellPaintListener;
	private MouseMoveListener shellMouseMoveListener;
	private MouseListener shellMouseListener;
	
	private boolean isTouch;
	private boolean isGrabbedShell;
	private Point grabPosition;

	public SpecialKeyWindow(EmulatorSkin skin, String layoutName) {
		super(skin.getShell(), SWT.RIGHT | SWT.CENTER);

		this.skin = skin;
		this.parent = skin.getShell();
		if (SwtUtil.isMacPlatform() == false) {
			this.shell = new Shell(parent,
					SWT.NO_TRIM | SWT.RESIZE | SWT.TOOL);
		} else {
			this.shell = new Shell(parent.getDisplay(),
					SWT.NO_TRIM | SWT.RESIZE | SWT.TOOL);
		}

		this.communicator = skin.communicator;
		this.grabPosition = new Point(0, 0);

		shell.setText(parent.getText());
		shell.setImage(parent.getImage());

		/* load dbi file */
		String skinPath = skin.skinInfo.getSkinPath();
		String specialKeyWindowPath = skinPath + File.separator + KEYWINDOW_LAYOUT + File.separator + layoutName;
		logger.info("special key window path : " + specialKeyWindowPath);
		EmulatorUI dbiContents = loadXMLForKeyWindow(specialKeyWindowPath);

		/* image init */
		this.imageRegistry = new SpecialKeyWindowImageRegistry(shell.getDisplay(), dbiContents, specialKeyWindowPath);
		
		/* get keywindow image */
		keyWindowImage = imageRegistry.getSpecialKeyWindowImage(EmulatorConfig.DEFAULT_WINDOW_ROTATION, SpecailKeyWindowImageType.SPECIAL_IMAGE_TYPE_NORMAL);		
		keyWindowPressedImage = imageRegistry.getSpecialKeyWindowImage(EmulatorConfig.DEFAULT_WINDOW_ROTATION, SpecailKeyWindowImageType.SPECIAL_IMAGE_TYPE_PRESSED);
		
		SpecialKeyWindowUtil.trimShell(shell, keyWindowImage);
		SpecialKeyWindowUtil.trimShell(shell, keyWindowPressedImage);
		
		/* calculate the key window size */
		widthBase = keyWindowImage.getImageData().width;
		heightBase = keyWindowImage.getImageData().height;

		/* make a frame image */
		this.colorFrame = new Color(shell.getDisplay(), new RGB(38, 38, 38));

		shell.setBackground(colorFrame);

		addKeyWindowListener();

		shell.setSize(widthBase, heightBase);
	}

	private EmulatorUI loadXMLForKeyWindow(String skinPath) {
		String dbiPath = skinPath + File.separator + DBI_FILE_NAME;
		logger.info("load dbi file from " + dbiPath);

		FileInputStream fis = null;
		EmulatorUI emulatorUI = null;

		try {
			fis = new FileInputStream(dbiPath);
			logger.info("============ dbi contents ============");
			byte[] bytes = IOUtil.getBytes(fis);
			logger.info(new String(bytes, "UTF-8"));
			logger.info("=======================================");

			emulatorUI = JaxbUtil.unmarshal(bytes, EmulatorUI.class);
		} catch (IOException e) {
			logger.log(Level.SEVERE, e.getMessage(), e);
		} catch (JaxbException e) {
			logger.log(Level.SEVERE, e.getMessage(), e);
		} finally {
			IOUtil.close(fis);
		}

		return emulatorUI;
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
					logger.info("MouseMove in SpecialKeyWindow : " + e.x + ", " + e.y);
					HWKey pressedHWKey = currentPressedHWKey;					
					int x = pressedHWKey.getRegion().x;
					int y = pressedHWKey.getRegion().y;
					int width = pressedHWKey.getRegion().width;
					int height = pressedHWKey.getRegion().height;
					int eventType;

					if (SpecialKeyWindowUtil.isInGeometry(e.x, e.y, x, y, width, height)) {
						eventType = MouseEventType.DRAG.value();
					} else {
						isTouch = false;
						eventType = MouseEventType.RELEASE.value();
						/* rollback a keyPressed image resion */
						shell.redraw(x, y, width, height, false);
					}

					MouseEventData mouseEventData = new MouseEventData(
							MouseButtonType.LEFT.value(), MouseEventType.DRAG.value(), e.x, e.y, e.x, e.y, 0);
					communicator.sendToQEMU(SendCommand.SEND_MOUSE_EVENT, mouseEventData, false);
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
					logger.info("MouseUp in SpecialKeyWindow : " + e.x + ", " + e.y);
					isGrabbedShell = false;
					grabPosition.x = grabPosition.y = 0;
					HWKey pressedHWKey = currentPressedHWKey;

					if (pressedHWKey != null && pressedHWKey.getKeyCode() != SpecialKeyWindowUtil.UNKNOWN_KEYCODE) {
						/* send event */
						if (isTouch) {
							isTouch = false;
							MouseEventData mouseEventData = new MouseEventData(
									MouseButtonType.LEFT.value(), MouseEventType.RELEASE.value(), e.x, e.y, e.x, e.y, 0);
							communicator.sendToQEMU(SendCommand.SEND_MOUSE_EVENT, mouseEventData, false);
						} else {
							KeyEventData keyEventData = new KeyEventData(KeyEventType.RELEASED.value(), pressedHWKey.getKeyCode(), 0, 0);
							communicator.sendToQEMU(SendCommand.SEND_HARD_KEY_EVENT, keyEventData, false);
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
					logger.info("MouseDown in SpecialKeyWindow : " + e.x + ", " + e.y);

					/* HW key handling */
					final HWKey hwKey = SpecialKeyWindowUtil.getHWKey(e.x, e.y, EmulatorConfig.DEFAULT_WINDOW_ROTATION);
					if (hwKey == null) {		
						isGrabbedShell = true;
						grabPosition.x = e.x;
						grabPosition.y = e.y;
						return;						
					}

					if (hwKey.getKeyCode() != SpecialKeyWindowUtil.UNKNOWN_KEYCODE) {
						if (hwKey.getTooltip().equalsIgnoreCase("touch")) {
							isTouch = true;
							MouseEventData mouseEventData = new MouseEventData(
									MouseButtonType.LEFT.value(), MouseEventType.PRESS.value(), e.x, e.y, e.x, e.y, 0);
							communicator.sendToQEMU(SendCommand.SEND_MOUSE_EVENT, mouseEventData, false);
						} else {
							/* send event */
							KeyEventData keyEventData = new KeyEventData(KeyEventType.PRESSED.value(), hwKey.getKeyCode(), 0, 0);
							communicator.sendToQEMU(SendCommand.SEND_HARD_KEY_EVENT, keyEventData, false);
						}

						currentPressedHWKey = hwKey;
						shell.setToolTipText(null);

						/* draw the HW key region as the cropped keyPressed image area */
						if(hwKey.getRegion() != null &&
								hwKey.getRegion().width != 0 && hwKey.getRegion().height != 0) {
							shell.getDisplay().syncExec(new Runnable() {
								public void run() {
									if(keyWindowPressedImage != null) {
										GC gc = new GC(shell);
										if(gc != null) {
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

		colorFrame.dispose();
		imageRegistry.dispose();
	}
}
