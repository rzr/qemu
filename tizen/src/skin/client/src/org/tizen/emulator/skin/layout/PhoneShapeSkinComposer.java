/**
 * 
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
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Shell;
import org.tizen.emulator.skin.EmulatorSkinState;
import org.tizen.emulator.skin.comm.ICommunicator.KeyEventType;
import org.tizen.emulator.skin.comm.ICommunicator.SendCommand;
import org.tizen.emulator.skin.comm.sock.SocketCommunicator;
import org.tizen.emulator.skin.comm.sock.data.KeyEventData;
import org.tizen.emulator.skin.config.EmulatorConfig;
import org.tizen.emulator.skin.config.EmulatorConfig.ArgsConstants;
import org.tizen.emulator.skin.config.EmulatorConfig.SkinPropertiesConstants;
import org.tizen.emulator.skin.dbi.LcdType;
import org.tizen.emulator.skin.dbi.RegionType;
import org.tizen.emulator.skin.dbi.RotationType;
import org.tizen.emulator.skin.image.ImageRegistry;
import org.tizen.emulator.skin.image.ImageRegistry.IconName;
import org.tizen.emulator.skin.image.ImageRegistry.ImageType;
import org.tizen.emulator.skin.log.SkinLogger;
import org.tizen.emulator.skin.util.SkinRotation;
import org.tizen.emulator.skin.util.SkinUtil;
import org.tizen.emulator.skin.util.SwtUtil;

public class PhoneShapeSkinComposer implements ISkinComposer {
	private Logger logger = SkinLogger.getSkinLogger(
			PhoneShapeSkinComposer.class).getLogger();

	private EmulatorConfig config;
	private Shell shell;
	private Canvas lcdCanvas;
	private EmulatorSkinState currentState;
	private ImageRegistry imageRegistry;
	private SocketCommunicator communicator;

	private PaintListener shellPaintListener;
	private MouseTrackListener shellMouseTrackListener;
	private MouseMoveListener shellMouseMoveListener;
	private MouseListener shellMouseListener;

	private boolean isGrabbedShell;
	private Point grabPosition;

	public PhoneShapeSkinComposer(EmulatorConfig config, Shell shell,
			EmulatorSkinState currentState, ImageRegistry imageRegistry,
			SocketCommunicator communicator) {
		this.config = config;
		this.shell = shell;
		this.currentState = currentState;
		this.imageRegistry = imageRegistry;
		this.communicator = communicator;
		this.isGrabbedShell= false;
		this.grabPosition = new Point(0, 0);
	}

	@Override
	public Canvas compose() {
		lcdCanvas = new Canvas(shell, SWT.EMBEDDED); //TODO:

		int x = config.getSkinPropertyInt(SkinPropertiesConstants.WINDOW_X,
				EmulatorConfig.DEFAULT_WINDOW_X);
		int y = config.getSkinPropertyInt(SkinPropertiesConstants.WINDOW_Y,
				EmulatorConfig.DEFAULT_WINDOW_Y);

		currentState.setCurrentResolutionWidth(
				config.getArgInt(ArgsConstants.RESOLUTION_WIDTH));
		currentState.setCurrentResolutionHeight(
				config.getArgInt(ArgsConstants.RESOLUTION_HEIGHT));

		int scale = SkinUtil.getValidScale(config);
//		int rotationId = config.getPropertyShort( PropertiesConstants.WINDOW_ROTATION,
//				EmulatorConfig.DEFAULT_WINDOW_ROTATION );
		// has to be portrait mode at first booting time
		short rotationId = EmulatorConfig.DEFAULT_WINDOW_ROTATION;

		composeInternal(lcdCanvas, x, y, scale, rotationId);
		logger.info("resolution : " + currentState.getCurrentResolution() +
				", scale : " + scale);

		return lcdCanvas;
	}

	@Override
	public void composeInternal(Canvas lcdCanvas,
			int x, int y, int scale, short rotationId) {

		//shell.setBackground(shell.getDisplay().getSystemColor(SWT.COLOR_BLACK));
		shell.setLocation(x, y);

		String emulatorName = SkinUtil.makeEmulatorName(config);
		shell.setText(emulatorName);

		lcdCanvas.setBackground(shell.getDisplay().getSystemColor(SWT.COLOR_BLACK));

		if (SwtUtil.isWindowsPlatform()) {
			shell.setImage(imageRegistry.getIcon(IconName.EMULATOR_TITLE_ICO));
		} else {
			shell.setImage(imageRegistry.getIcon(IconName.EMULATOR_TITLE));
		}

		arrangeSkin(scale, rotationId);

		if (currentState.getCurrentImage() == null) {
			logger.severe("Failed to load initial skin image file. Kill this skin process.");
			SkinUtil.openMessage(shell, null,
					"Failed to load Skin image file.", SWT.ICON_ERROR, config);
			System.exit(-1);
		}
	}

	@Override
	public void arrangeSkin(int scale, short rotationId) {
		currentState.setCurrentScale(scale);
		currentState.setCurrentRotationId(rotationId);
		currentState.setCurrentAngle(SkinRotation.getAngle(rotationId));

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
				imageRegistry, shell, rotationId, scale, ImageType.IMG_TYPE_MAIN));
		currentState.setCurrentKeyPressedImag(SkinUtil.createScaledImage(
				imageRegistry, shell, rotationId, scale, ImageType.IMG_TYPE_PRESSED));

		if (tempImage != null) {
			tempImage.dispose();
		}
		if (tempKeyPressedImage != null) {
			tempKeyPressedImage.dispose();
		}

		/* custom window shape */
		SkinUtil.trimShell(shell, currentState.getCurrentImage());

		/* set window size */
		if (currentState.getCurrentImage() != null) {
			ImageData imageData = currentState.getCurrentImage().getImageData();
			shell.setMinimumSize(imageData.width, imageData.height);
			shell.setSize(imageData.width, imageData.height);
		}

		shell.redraw();
		shell.pack();

		/* arrange the lcd */
		Rectangle lcdBounds = adjustLcdGeometry(lcdCanvas,
				currentState.getCurrentResolutionWidth(),
				currentState.getCurrentResolutionHeight(), scale, rotationId);

		if (lcdBounds == null) {
			logger.severe("Failed to lcd information for phone shape skin.");
			SkinUtil.openMessage(shell, null,
					"Failed to read lcd information for phone shape skin.\n" +
					"Check the contents of skin dbi file.",
					SWT.ICON_ERROR, config);
			System.exit(-1);
		}
		logger.info("lcd bounds : " + lcdBounds);

		lcdCanvas.setBounds(lcdBounds);
	}

	@Override
	public Rectangle adjustLcdGeometry(
			Canvas lcdCanvas, int resolutionW, int resolutionH,
			int scale, short rotationId) {
		Rectangle lcdBounds = new Rectangle(0, 0, 0, 0);

		float convertedScale = SkinUtil.convertScale(scale);
		RotationType rotation = SkinRotation.getRotation(rotationId);

		LcdType lcd = rotation.getLcd(); /* from dbi */
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

	public void addPhoneShapeListener(final Shell shell) {
		shellPaintListener = new PaintListener() {
			@Override
			public void paintControl(final PaintEvent e) {
				/* general shell does not support native transparency,
				 * so draw image with GC. */
				if (currentState.getCurrentImage() != null) {
					e.gc.drawImage(currentState.getCurrentImage(), 0, 0);
				}

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
					return;
				}

				final HWKey hwKey = SkinUtil.getHWKey(e.x, e.y,
						currentState.getCurrentRotationId(), currentState.getCurrentScale());
				if (hwKey == null) {
					/* remove hover */
					HWKey hoveredHWKey = currentState.getCurrentHoveredHWKey();

					if (hoveredHWKey != null) {
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
				if (currentState.getCurrentHoveredHWKey() == null &&
						hwKey.getTooltip().isEmpty() == false) {
					shell.setToolTipText(hwKey.getTooltip());
				}

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

								currentState.setCurrentHoveredHWKey(hwKey);
							}
						}
					}
				});

			}
		};

		shell.addMouseMoveListener(shellMouseMoveListener);

		shellMouseListener = new MouseListener() {
			@Override
			public void mouseUp(MouseEvent e) {
				if (e.button == 1) { /* left button */
					logger.info("mouseUp in Skin");

					isGrabbedShell = false;
					grabPosition.x = grabPosition.y = 0;

					HWKey pressedHWKey = currentState.getCurrentPressedHWKey();
					if (pressedHWKey == null) {
						return;
					}

					if (pressedHWKey.getKeyCode() != SkinUtil.UNKNOWN_KEYCODE) {
						/* send event */
						KeyEventData keyEventData = new KeyEventData(
								KeyEventType.RELEASED.value(), pressedHWKey.getKeyCode(), 0, 0);
						communicator.sendToQEMU(SendCommand.SEND_HARD_KEY_EVENT, keyEventData);

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
					logger.info("mouseDown in Skin");

					isGrabbedShell = true;
					grabPosition.x = e.x;
					grabPosition.y = e.y;

					final HWKey hwKey = SkinUtil.getHWKey(e.x, e.y,
							currentState.getCurrentRotationId(), currentState.getCurrentScale());
					if (hwKey == null) {
						return;
					}

					if (hwKey.getKeyCode() != SkinUtil.UNKNOWN_KEYCODE) {
						/* send event */
						KeyEventData keyEventData = new KeyEventData(
								KeyEventType.PRESSED.value(), hwKey.getKeyCode(), 0, 0);
						communicator.sendToQEMU(SendCommand.SEND_HARD_KEY_EVENT, keyEventData);

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
													hwKey.getRegion().width, hwKey.getRegion().height, //src
													hwKey.getRegion().x, hwKey.getRegion().y,
													hwKey.getRegion().width, hwKey.getRegion().height); //dst

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
	}
}
