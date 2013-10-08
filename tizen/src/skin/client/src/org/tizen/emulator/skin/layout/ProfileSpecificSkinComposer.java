/**
 * Profile-Specific Skin Layout
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

package org.tizen.emulator.skin.layout;

import java.util.logging.Logger;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.events.MouseMoveListener;
import org.eclipse.swt.events.MouseTrackAdapter;
import org.eclipse.swt.events.MouseTrackListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Shell;
import org.tizen.emulator.skin.EmulatorSkin;
import org.tizen.emulator.skin.EmulatorSkinState;
import org.tizen.emulator.skin.comm.ICommunicator.KeyEventType;
import org.tizen.emulator.skin.comm.ICommunicator.SendCommand;
import org.tizen.emulator.skin.comm.sock.SocketCommunicator;
import org.tizen.emulator.skin.comm.sock.data.KeyEventData;
import org.tizen.emulator.skin.config.EmulatorConfig;
import org.tizen.emulator.skin.config.EmulatorConfig.ArgsConstants;
import org.tizen.emulator.skin.config.EmulatorConfig.SkinPropertiesConstants;
import org.tizen.emulator.skin.custom.CustomProgressBar;
import org.tizen.emulator.skin.dbi.DisplayType;
import org.tizen.emulator.skin.dbi.RegionType;
import org.tizen.emulator.skin.dbi.RotationType;
import org.tizen.emulator.skin.image.ImageRegistry.IconName;
import org.tizen.emulator.skin.image.ProfileSkinImageRegistry;
import org.tizen.emulator.skin.image.ProfileSkinImageRegistry.SkinImageType;
import org.tizen.emulator.skin.log.SkinLogger;
import org.tizen.emulator.skin.menu.PopupMenu;
import org.tizen.emulator.skin.util.SkinRotation;
import org.tizen.emulator.skin.util.SkinUtil;
import org.tizen.emulator.skin.util.SwtUtil;

public class ProfileSpecificSkinComposer implements ISkinComposer {
	private Logger logger = SkinLogger.getSkinLogger(
			ProfileSpecificSkinComposer.class).getLogger();

	private EmulatorConfig config;
	private EmulatorSkin skin;
	private Shell shell;
	private Canvas lcdCanvas;
	private EmulatorSkinState currentState;
	private SocketCommunicator communicator;

	private PaintListener shellPaintListener;
	private MouseTrackListener shellMouseTrackListener;
	private MouseMoveListener shellMouseMoveListener;
	private MouseListener shellMouseListener;

	private ProfileSkinImageRegistry imageRegistry;
	private boolean isGrabbedShell;
	private Point grabPosition;

	public ProfileSpecificSkinComposer(
			EmulatorConfig config, EmulatorSkin skin) {
		this.config = config;
		this.skin = skin;
		this.shell = skin.getShell();
		this.currentState = skin.getEmulatorSkinState();
		this.communicator = skin.communicator;

		this.isGrabbedShell= false;
		this.grabPosition = new Point(0, 0);

		this.imageRegistry = new ProfileSkinImageRegistry(
				shell.getDisplay(), config.getDbiContents(), skin.skinInfo.getSkinPath());
	}

	@Override
	public Canvas compose(int style) {
		lcdCanvas = new Canvas(shell, style);

		int vmIndex =
				config.getArgInt(ArgsConstants.VM_BASE_PORT) % 100;
		int x = config.getSkinPropertyInt(SkinPropertiesConstants.WINDOW_X,
				EmulatorConfig.DEFAULT_WINDOW_X + vmIndex);
		int y = config.getSkinPropertyInt(SkinPropertiesConstants.WINDOW_Y,
				EmulatorConfig.DEFAULT_WINDOW_Y + vmIndex);

		int scale = currentState.getCurrentScale();
		short rotationId = currentState.getCurrentRotationId();

		composeInternal(lcdCanvas, x, y, scale, rotationId);
		logger.info("resolution : " + currentState.getCurrentResolution() +
				", scale : " + scale);

		return lcdCanvas;
	}

	@Override
	public void composeInternal(Canvas lcdCanvas,
			int x, int y, int scale, short rotationId) {
		shell.setLocation(x, y);

		/* This string must match the definition of Emulator-Manager */
		String emulatorName = SkinUtil.makeEmulatorName(config);
		shell.setText("Emulator - " + emulatorName);

		lcdCanvas.setBackground(
				shell.getDisplay().getSystemColor(SWT.COLOR_BLACK));

		if (SwtUtil.isWindowsPlatform()) {
			shell.setImage(skin.getImageRegistry()
					.getIcon(IconName.EMULATOR_TITLE_ICO));
		} else {
			shell.setImage(skin.getImageRegistry()
					.getIcon(IconName.EMULATOR_TITLE));
		}

		/* create a progress bar for booting status */
		skin.bootingProgress = new CustomProgressBar(shell, SWT.NONE);
		skin.bootingProgress.setBackground(
				new Color(shell.getDisplay(), new RGB(38, 38, 38)));

		arrangeSkin(scale, rotationId);

		if (currentState.getCurrentImage() == null) {
			logger.severe("Failed to load initial skin image file. Kill this skin process.");
			SkinUtil.openMessage(shell, null,
					"Failed to load Skin image file.", SWT.ICON_ERROR, config);
			System.exit(-1);
		}

		/* open the key window if key window menu item was enabled */
		PopupMenu popupMenu = skin.getPopupMenu();

		if (popupMenu != null && popupMenu.keyWindowItem != null) {
			final int dockValue = config.getSkinPropertyInt(
					SkinPropertiesConstants.KEYWINDOW_POSITION, 0);

			shell.getDisplay().asyncExec(new Runnable() {
				@Override
				public void run() {
					if (dockValue == 0 || dockValue == SWT.NONE) {
						skin.getKeyWindowKeeper().openKeyWindow(
								SWT.RIGHT | SWT.CENTER, false);
					} else {
						skin.getKeyWindowKeeper().openKeyWindow(
								dockValue, false);
					}
				}
			});
		}
	}

	@Override
	public void arrangeSkin(int scale, short rotationId) {
		currentState.setCurrentScale(scale);
		currentState.setCurrentRotationId(rotationId);
		currentState.setCurrentAngle(SkinRotation.getAngle(rotationId));

		/* arrange the display */
		Rectangle lcdBounds = adjustLcdGeometry(lcdCanvas,
				currentState.getCurrentResolutionWidth(),
				currentState.getCurrentResolutionHeight(), scale, rotationId);

		if (lcdBounds == null) {
			logger.severe("Failed to read display information for skin.");
			SkinUtil.openMessage(shell, null,
					"Failed to read display information for skin.\n" +
					"Check the contents of skin dbi file.",
					SWT.ICON_ERROR, config);
			System.exit(-1);
		}
		logger.info("display bounds : " + lcdBounds);

		currentState.setDisplayBounds(lcdBounds);

		/* arrange the skin image */
		Image tempImage = null;
		Image tempKeyPressedImage = null;

		if (currentState.getCurrentImage() != null) {
			tempImage = currentState.getCurrentImage();
		}
		if (currentState.getCurrentKeyPressedImage() != null) {
			tempKeyPressedImage = currentState.getCurrentKeyPressedImage();
		}

		currentState.setCurrentImage(SkinUtil.createScaledImage(
				shell.getDisplay(), imageRegistry,
				SkinImageType.PROFILE_IMAGE_TYPE_NORMAL,
				rotationId, scale));
		currentState.setCurrentKeyPressedImage(SkinUtil.createScaledImage(
				shell.getDisplay(), imageRegistry,
				SkinImageType.PROFILE_IMAGE_TYPE_PRESSED,
				rotationId, scale));

		if (tempImage != null) {
			tempImage.dispose();
		}
		if (tempKeyPressedImage != null) {
			tempKeyPressedImage.dispose();
		}

		if (SwtUtil.isMacPlatform() == true) {
			lcdCanvas.setBounds(currentState.getDisplayBounds());
		}

		/* arrange the progress bar */
		if (skin.bootingProgress != null) {
			skin.bootingProgress.setBounds(lcdBounds.x,
					lcdBounds.y + lcdBounds.height + 1, lcdBounds.width, 2);
		}

		/* set window size */
		if (currentState.getCurrentImage() != null) {
			ImageData imageData = currentState.getCurrentImage().getImageData();
			shell.setSize(imageData.width, imageData.height);
		}

		/* custom window shape */
		SkinUtil.trimShell(shell, currentState.getCurrentImage());

		currentState.setNeedToUpdateDisplay(true);
		shell.redraw();
	}

	@Override
	public Rectangle adjustLcdGeometry(
			Canvas lcdCanvas, int resolutionW, int resolutionH,
			int scale, short rotationId) {
		Rectangle lcdBounds = new Rectangle(0, 0, 0, 0);

		float convertedScale = SkinUtil.convertScale(scale);
		RotationType rotation = SkinRotation.getRotation(rotationId);

		DisplayType lcd = rotation.getDisplay(); /* from dbi */
		if (lcd == null) {
			return null;
		}

		RegionType region = lcd.getRegion();
		if (region == null) {
			return null;
		}

		Integer left = region.getLeft();
		Integer top = region.getTop();
		Integer width = region.getWidth();
		Integer height = region.getHeight();

		lcdBounds.x = (int) (left * convertedScale);
		lcdBounds.y = (int) (top * convertedScale);
		lcdBounds.width = (int) (width * convertedScale);
		lcdBounds.height = (int) (height * convertedScale);

		return lcdBounds;
	}

	public void addProfileSpecificListener(final Shell shell) {
		shellPaintListener = new PaintListener() {
			@Override
			public void paintControl(final PaintEvent e) {
				if (currentState.isNeedToUpdateDisplay() == true) {
					currentState.setNeedToUpdateDisplay(false);

					if (SwtUtil.isMacPlatform() == false) {
						lcdCanvas.setBounds(currentState.getDisplayBounds());
					}
				}

				/* set window size once again (for ubuntu 12.04) */
				if (currentState.getCurrentImage() != null) {
					ImageData imageData = currentState.getCurrentImage().getImageData();
					shell.setSize(imageData.width, imageData.height);
				}

				/* general shell does not support native transparency,
				 so draw image with GC */
				if (currentState.getCurrentImage() != null) {
					e.gc.drawImage(currentState.getCurrentImage(), 0, 0);
				}

				skin.getKeyWindowKeeper().redock(false, false);
			}
		};

		shell.addPaintListener(shellPaintListener);

		shellMouseTrackListener = new MouseTrackAdapter() {
			@Override
			public void mouseExit(MouseEvent e) {
				/* shell does not receive event only with MouseMoveListener
				 * in case that : hover hardkey -> mouse move into LCD area */
				HWKey hoveredHWKey = currentState.getCurrentHoveredHWKey();

				if (hoveredHWKey != null) {
					shell.redraw(hoveredHWKey.getRegion().x,
							hoveredHWKey.getRegion().y,
							hoveredHWKey.getRegion().width,
							hoveredHWKey.getRegion().height, false);

					currentState.setCurrentHoveredHWKey(null);
					shell.setToolTipText(null);
				}
			}
		};

		shell.addMouseTrackListener(shellMouseTrackListener);

		shellMouseMoveListener = new MouseMoveListener() {
			@Override
			public void mouseMove(MouseEvent e) {
				if (isGrabbedShell == true && e.button == 0/* left button */ &&
						currentState.getCurrentPressedHWKey() == null) {
					/* move a window */
					Point previousLocation = shell.getLocation();
					int x = previousLocation.x + (e.x - grabPosition.x);
					int y = previousLocation.y + (e.y - grabPosition.y);

					shell.setLocation(x, y);

					skin.getKeyWindowKeeper().redock(false, false);

					return;
				}

				final HWKey hwKey = SkinUtil.getHWKey(e.x, e.y,
						currentState.getCurrentRotationId(), currentState.getCurrentScale());
				final HWKey hoveredHWKey = currentState.getCurrentHoveredHWKey();

				if (hwKey == null) {
					if (hoveredHWKey != null) {
						/* remove hover */
						shell.redraw(hoveredHWKey.getRegion().x,
								hoveredHWKey.getRegion().y,
								hoveredHWKey.getRegion().width,
								hoveredHWKey.getRegion().height, false);

						currentState.setCurrentHoveredHWKey(null);
						shell.setToolTipText(null);
					}

					return;
				}

				/* register a tooltip */
				if (hoveredHWKey == null &&
						hwKey.getTooltip().isEmpty() == false) {
					shell.setToolTipText(hwKey.getTooltip());

					currentState.setCurrentHoveredHWKey(hwKey);

					/* draw hover */
					shell.getDisplay().syncExec(new Runnable() {
						public void run() {
							if (hwKey.getRegion().width != 0 && hwKey.getRegion().height != 0) {
								GC gc = new GC(shell);
								if (gc != null) {
									gc.setLineWidth(1);
									gc.setForeground(currentState.getHoverColor());
									gc.drawRectangle(hwKey.getRegion().x, hwKey.getRegion().y,
											hwKey.getRegion().width - 1, hwKey.getRegion().height - 1);

									gc.dispose();
								}
							}
						}
					});
				} else {
					if (hwKey.getRegion().x != hoveredHWKey.getRegion().x ||
							hwKey.getRegion().y != hoveredHWKey.getRegion().y) {
						/* remove hover */
						shell.redraw(hoveredHWKey.getRegion().x,
								hoveredHWKey.getRegion().y,
								hoveredHWKey.getRegion().width,
								hoveredHWKey.getRegion().height, false);

						currentState.setCurrentHoveredHWKey(null);
						shell.setToolTipText(null);
					}
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

					skin.getKeyWindowKeeper().redock(false, true);

					/* HW key handling */
					HWKey pressedHWKey = currentState.getCurrentPressedHWKey();
					if (pressedHWKey == null) {
						logger.info("mouseUp in Skin : " + e.x + ", " + e.y);
						return;
					}

					if (pressedHWKey.getKeyCode() != SkinUtil.UNKNOWN_KEYCODE) {
						logger.info(pressedHWKey.getName() + " key is released");

						/* send event */
						KeyEventData keyEventData = new KeyEventData(
								KeyEventType.RELEASED.value(), pressedHWKey.getKeyCode(), 0, 0);
						communicator.sendToQEMU(
								SendCommand.SEND_HARD_KEY_EVENT, keyEventData, false);

						currentState.setCurrentPressedHWKey(null);

						/* roll back a keyPressed image region */
						shell.redraw(pressedHWKey.getRegion().x, pressedHWKey.getRegion().y,
								pressedHWKey.getRegion().width, pressedHWKey.getRegion().height, false);

						if (pressedHWKey.getKeyCode() != 101) { // TODO: not necessary for home key
							SkinUtil.trimShell(shell, currentState.getCurrentImage(),
									pressedHWKey.getRegion().x, pressedHWKey.getRegion().y,
									pressedHWKey.getRegion().width, pressedHWKey.getRegion().height);
						}
					}
				}
			}

			@Override
			public void mouseDown(MouseEvent e) {
				if (1 == e.button) { /* left button */
					/* HW key handling */
					final HWKey hwKey = SkinUtil.getHWKey(e.x, e.y,
							currentState.getCurrentRotationId(), currentState.getCurrentScale());
					if (hwKey == null) {
						logger.info("mouseDown in Skin : " + e.x + ", " + e.y);

						isGrabbedShell = true;
						grabPosition.x = e.x;
						grabPosition.y = e.y;

						return;
					}

					if (hwKey.getKeyCode() != SkinUtil.UNKNOWN_KEYCODE) {
						logger.info(hwKey.getName() + " key is pressed");

						/* send event */
						KeyEventData keyEventData = new KeyEventData(
								KeyEventType.PRESSED.value(), hwKey.getKeyCode(), 0, 0);
						communicator.sendToQEMU(
								SendCommand.SEND_HARD_KEY_EVENT, keyEventData, false);

						currentState.setCurrentPressedHWKey(hwKey);

						shell.setToolTipText(null);

						/* draw the HW key region as the cropped keyPressed image area */
						if (hwKey.getRegion() != null &&
								hwKey.getRegion().width != 0 && hwKey.getRegion().height != 0) {
							shell.getDisplay().syncExec(new Runnable() {
								public void run() {
									if (currentState.getCurrentKeyPressedImage() != null) {
										GC gc = new GC(shell);
										if (gc != null) {
											gc.drawImage(currentState.getCurrentKeyPressedImage(),
													hwKey.getRegion().x, hwKey.getRegion().y,
													hwKey.getRegion().width, hwKey.getRegion().height, /* src */
													hwKey.getRegion().x, hwKey.getRegion().y,
													hwKey.getRegion().width, hwKey.getRegion().height); /* dst */

											gc.dispose();

											if (hwKey.getKeyCode() != 101) { // TODO: not necessary for home key
												SkinUtil.trimShell(shell, currentState.getCurrentKeyPressedImage(),
														hwKey.getRegion().x, hwKey.getRegion().y,
														hwKey.getRegion().width, hwKey.getRegion().height);
											}

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

	@Override
	public void composerFinalize() {
		if (null != shellPaintListener) {
			shell.removePaintListener(shellPaintListener);
		}

		if (null != shellMouseTrackListener) {
			shell.removeMouseTrackListener(shellMouseTrackListener);
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
