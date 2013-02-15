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
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.graphics.Region;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Shell;
import org.tizen.emulator.skin.EmulatorSkin;
import org.tizen.emulator.skin.EmulatorSkinState;
import org.tizen.emulator.skin.comm.ICommunicator.RotationInfo;
import org.tizen.emulator.skin.config.EmulatorConfig;
import org.tizen.emulator.skin.config.EmulatorConfig.ArgsConstants;
import org.tizen.emulator.skin.config.EmulatorConfig.SkinPropertiesConstants;
import org.tizen.emulator.skin.custom.CustomButton;
import org.tizen.emulator.skin.custom.CustomProgressBar;
import org.tizen.emulator.skin.image.ImageRegistry;
import org.tizen.emulator.skin.image.ImageRegistry.IconName;
import org.tizen.emulator.skin.log.SkinLogger;
import org.tizen.emulator.skin.util.SkinRotation;
import org.tizen.emulator.skin.util.SkinUtil;
import org.tizen.emulator.skin.util.SwtUtil;

public class GeneralPurposeSkinComposer implements ISkinComposer {
	private static final String PATCH_IMAGES_PATH = "images/emul-window/";
	private static final String TOGGLE_BUTTON_NORMAL_IMG = "arrow_nml.png";
	private static final String TOGGLE_BUTTON_HOVER_IMG = "arrow_hover.png";
	private static final String TOGGLE_BUTTON_PUSHED_IMG = "arrow_pushed.png";

	private static final int PAIR_TAG_POSITION_X = 26;
	private static final int PAIR_TAG_POSITION_Y = 13;
	private static final int PAIR_TAG_POSITION_WIDTH = 8;
	private static final int PAIR_TAG_POSITION_HEIGHT = 8;

	private Logger logger = SkinLogger.getSkinLogger(
			GeneralPurposeSkinComposer.class).getLogger();

	private EmulatorConfig config;
	private EmulatorSkin skin;
	private Shell shell;
	private Canvas displayCanvas;
	private Color backgroundColor;
	private CustomButton toggleButton;
	private EmulatorSkinState currentState;

	private ImageRegistry imageRegistry;
	private SkinPatches frameMaker;

	private PaintListener shellPaintListener;
	private MouseMoveListener shellMouseMoveListener;
	private MouseListener shellMouseListener;

	private boolean isGrabbedShell;
	private Point grabPosition;

	public GeneralPurposeSkinComposer(EmulatorConfig config, EmulatorSkin skin,
			Shell shell, EmulatorSkinState currentState,
			ImageRegistry imageRegistry) {
		this.config = config;
		this.skin = skin;
		this.shell = shell;
		this.currentState = currentState;
		this.imageRegistry = imageRegistry;
		this.isGrabbedShell= false;
		this.grabPosition = new Point(0, 0);

		this.frameMaker = new SkinPatches(PATCH_IMAGES_PATH);
		this.backgroundColor = new Color(shell.getDisplay(), new RGB(38, 38, 38));
	}

	@Override
	public Canvas compose(int style) {
		displayCanvas = new Canvas(shell, style);

		int vmIndex = config.getArgInt(ArgsConstants.NET_BASE_PORT) % 100;

		int x = config.getSkinPropertyInt(SkinPropertiesConstants.WINDOW_X,
				EmulatorConfig.DEFAULT_WINDOW_X + vmIndex);
		int y = config.getSkinPropertyInt(SkinPropertiesConstants.WINDOW_Y,
				EmulatorConfig.DEFAULT_WINDOW_Y + vmIndex);

		currentState.setCurrentResolutionWidth(
				config.getArgInt(ArgsConstants.RESOLUTION_WIDTH));
		currentState.setCurrentResolutionHeight(
				config.getArgInt(ArgsConstants.RESOLUTION_HEIGHT));

		int scale = SkinUtil.getValidScale(config);
		short rotationId = EmulatorConfig.DEFAULT_WINDOW_ROTATION;

		composeInternal(displayCanvas, x, y, scale, rotationId);
		logger.info("resolution : " + currentState.getCurrentResolution() +
				", scale : " + scale);

		return displayCanvas;
	}

	@Override
	public void composeInternal(Canvas displayCanvas,
			final int x, final int y, int scale, short rotationId) {

		//shell.setBackground(shell.getDisplay().getSystemColor(SWT.COLOR_BLACK));
		shell.setLocation(x, y);

		String emulatorName = SkinUtil.makeEmulatorName(config);
		shell.setText(emulatorName);

		displayCanvas.setBackground(
				shell.getDisplay().getSystemColor(SWT.COLOR_BLACK));

		if (SwtUtil.isWindowsPlatform()) {
			shell.setImage(imageRegistry.getIcon(IconName.EMULATOR_TITLE_ICO));
		} else {
			shell.setImage(imageRegistry.getIcon(IconName.EMULATOR_TITLE));
		}

		/* load image for toggle button of key window */
		ClassLoader loader = this.getClass().getClassLoader();
		Image imageNormal = new Image(shell.getDisplay(),
				loader.getResourceAsStream(PATCH_IMAGES_PATH + TOGGLE_BUTTON_NORMAL_IMG));
		Image imageHover = new Image(shell.getDisplay(),
				loader.getResourceAsStream(PATCH_IMAGES_PATH + TOGGLE_BUTTON_HOVER_IMG));
		Image imagePushed = new Image(shell.getDisplay(),
				loader.getResourceAsStream(PATCH_IMAGES_PATH + TOGGLE_BUTTON_PUSHED_IMG));

		/* create a toggle button of key window */
		toggleButton = new CustomButton(shell, SWT.DRAW_TRANSPARENT | SWT.NO_FOCUS,
				imageNormal, imageHover, imagePushed);
		toggleButton.setBackground(backgroundColor);

		toggleButton.addMouseListener(new MouseListener() {
			@Override
			public void mouseDown(MouseEvent e) {
				if (skin.isSelectKeyWindow() == true) {
					skin.closeKeyWindow();
					skin.recentlyDocked = SWT.RIGHT | SWT.CENTER;
				} else {
					skin.openKeyWindow(SWT.RIGHT | SWT.CENTER, true);
				}
			}

			@Override
			public void mouseUp(MouseEvent e) {
				/* do nothing */
			}

			@Override
			public void mouseDoubleClick(MouseEvent e) {
				/* do nothing */
			}
		});

		/* make a pair tag circle */
		skin.pairTagCanvas = new Canvas(shell, SWT.NO_FOCUS);
		skin.pairTagCanvas.setBackground(backgroundColor);

		skin.pairTagCanvas.addPaintListener(new PaintListener() {
			@Override
			public void paintControl(PaintEvent e) {
				if (skin.colorPairTag != null) {
					e.gc.setBackground(skin.colorPairTag);
					e.gc.setAntialias(SWT.ON);
					e.gc.fillOval(
							0, 0, PAIR_TAG_POSITION_WIDTH, PAIR_TAG_POSITION_HEIGHT);
				}
			}
		});
		skin.pairTagCanvas.setVisible(false);

		/* create a progress bar for booting status */
		skin.bootingProgress = new CustomProgressBar(shell, SWT.NONE);
		skin.bootingProgress.setBackground(backgroundColor);

		arrangeSkin(scale, rotationId);

		/* open the key window */
		shell.getDisplay().asyncExec(new Runnable() {
			@Override
			public void run() {
				skin.openKeyWindow(SWT.RIGHT | SWT.CENTER, false);
			}
		});
	}

	@Override
	public void arrangeSkin(int scale, short rotationId) {
		currentState.setCurrentScale(scale);
		currentState.setCurrentRotationId(rotationId);
		currentState.setCurrentAngle(SkinRotation.getAngle(rotationId));

		/* arrange the display */
		Rectangle displayBounds = adjustLcdGeometry(displayCanvas,
				currentState.getCurrentResolutionWidth(),
				currentState.getCurrentResolutionHeight(), scale, rotationId);

		if (displayBounds == null) {
			logger.severe("Failed to lcd information for phone shape skin.");
			SkinUtil.openMessage(shell, null,
					"Failed to read lcd information for phone shape skin.\n" +
					"Check the contents of skin dbi file.",
					SWT.ICON_ERROR, config);
			System.exit(-1);
		}
		logger.info("display bounds : " + displayBounds);

		currentState.setDisplayBounds(displayBounds);
		displayCanvas.setBounds(displayBounds);

		/* arrange the skin image */
		Image tempImage = null;

		if (currentState.getCurrentImage() != null) {
			tempImage = currentState.getCurrentImage();
		}

		currentState.setCurrentImage(
				frameMaker.getPatchedImage(displayBounds.width, displayBounds.height));

		if (tempImage != null) {
			tempImage.dispose();
		}

		/* arrange the toggle button of key window */
		toggleButton.setBounds(displayBounds.x + displayBounds.width,
				displayBounds.y + (displayBounds.height / 2) - (toggleButton.getImageSize().y / 2),
				toggleButton.getImageSize().x, toggleButton.getImageSize().y);

		/* arrange the progress bar */
		if (skin.bootingProgress != null) {
			skin.bootingProgress.setBounds(displayBounds.x,
					displayBounds.y + displayBounds.height + 1, displayBounds.width, 2);
		}

		/* arrange the pair tag */
		skin.pairTagCanvas.setBounds(
				PAIR_TAG_POSITION_X, PAIR_TAG_POSITION_Y,
				PAIR_TAG_POSITION_WIDTH, PAIR_TAG_POSITION_HEIGHT);

		/* set window size */
		if (currentState.getCurrentImage() != null) {
			ImageData imageData = currentState.getCurrentImage().getImageData();
			shell.setMinimumSize(imageData.width, imageData.height);
		}

		shell.pack();

		if (currentState.getCurrentImage() != null) {
			ImageData imageData = currentState.getCurrentImage().getImageData();
			shell.setSize(imageData.width, imageData.height);
		}

		/* custom window shape */
		trimPatchedShell(shell, currentState.getCurrentImage());

		shell.redraw();
	}

	@Override
	public Rectangle adjustLcdGeometry(
			Canvas displayCanvas, int resolutionW, int resolutionH,
			int scale, short rotationId) {

		Rectangle lcdBounds = new Rectangle(
				frameMaker.getPatchWidth(), frameMaker.getPatchHeight(), 0, 0);

		float convertedScale = SkinUtil.convertScale(scale);
		RotationInfo rotation = RotationInfo.getValue(rotationId);

		/* resoultion, that is lcd size in general skin mode */
		if (RotationInfo.LANDSCAPE == rotation ||
				RotationInfo.REVERSE_LANDSCAPE == rotation) {
			lcdBounds.width = (int)(resolutionH * convertedScale);
			lcdBounds.height = (int)(resolutionW * convertedScale);
		} else {
			lcdBounds.width = (int)(resolutionW * convertedScale);
			lcdBounds.height = (int)(resolutionH * convertedScale);
		}

		return lcdBounds;
	}

	public static void trimPatchedShell(Shell shell, Image image) {
		if (null == image) {
			return;
		}
		ImageData imageData = image.getImageData();

		int width = imageData.width;
		int height = imageData.height;

		Region region = new Region();
		region.add(new Rectangle(0, 0, width, height));

		int r = shell.getDisplay().getSystemColor(SWT.COLOR_MAGENTA).getRed();
		int g = shell.getDisplay().getSystemColor(SWT.COLOR_MAGENTA).getGreen();
		int b = shell.getDisplay().getSystemColor(SWT.COLOR_MAGENTA).getBlue();
		int colorKey = 0;

		if (SwtUtil.isWindowsPlatform()) {
			colorKey = r << 24 | g << 16 | b << 8;
		} else {
			colorKey = r << 16 | g << 8 | b;
		}

		for (int i = 0; i < width; i++) {
			for (int j = 0; j < height; j++) {
				int colorPixel = imageData.getPixel(i, j);
				if (colorPixel == colorKey /* magenta */) {
					region.subtract(i, j, 1, 1);
				}
			}
		}

		shell.setRegion(region);
	}

	public void addGeneralPurposeListener(final Shell shell) {
		shellPaintListener = new PaintListener() {
			@Override
			public void paintControl(final PaintEvent e) {
				/* general shell does not support native transparency,
				 * so draw image with GC. */
				if (currentState.getCurrentImage() != null) {
					e.gc.drawImage(currentState.getCurrentImage(), 0, 0);
				}

				if (skin.keyWindow != null &&
						skin.keyWindow.getDockPosition() != SWT.NONE) {
					skin.keyWindow.dock(
							skin.keyWindow.getDockPosition(), false, false);
				}
			}
		};

		shell.addPaintListener(shellPaintListener);

		shellMouseMoveListener = new MouseMoveListener() {
			@Override
			public void mouseMove(MouseEvent e) {
				if (isGrabbedShell == true && e.button == 0/* left button */) {
					/* move a window */
					Point previousLocation = shell.getLocation();
					int x = previousLocation.x + (e.x - grabPosition.x);
					int y = previousLocation.y + (e.y - grabPosition.y);

					shell.setLocation(x, y);

					if (skin.keyWindow != null &&
							skin.keyWindow.getDockPosition() != SWT.NONE) {
						skin.keyWindow.dock(
								skin.keyWindow.getDockPosition(), false, false);
					}
				}
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

					if (skin.keyWindow != null &&
							skin.keyWindow.getDockPosition() != SWT.NONE) {
						skin.keyWindow.dock(
								skin.keyWindow.getDockPosition(), false, true);
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
				}
			}

			@Override
			public void mouseDoubleClick(MouseEvent e) {
				/* do nothing */
			}
		};

		shell.addMouseListener(shellMouseListener);
	}

//	private void createHWKeyRegion() {
//		if (compositeBase != null) {
//			compositeBase.dispose();
//			compositeBase = null;
//		}
//
//		List<KeyMapType> keyMapList =
//				SkinUtil.getHWKeyMapList(currentState.getCurrentRotationId());
//
//		if (keyMapList != null && keyMapList.isEmpty() == false) {
//			compositeBase = new Composite(shell, SWT.NONE);
//			compositeBase.setLayout(new GridLayout(1, true));
//
//			for (KeyMapType keyEntry : keyMapList) {
//				Button hardKeyButton = new Button(compositeBase, SWT.FLAT);
//				hardKeyButton.setText(keyEntry.getEventInfo().getKeyName());
//				hardKeyButton.setToolTipText(keyEntry.getTooltip());
//
//				hardKeyButton.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false));
//
//				final int keycode = keyEntry.getEventInfo().getKeyCode();
//				hardKeyButton.addMouseListener(new MouseListener() {
//					@Override
//					public void mouseDown(MouseEvent e) {
//						KeyEventData keyEventData = new KeyEventData(
//								KeyEventType.PRESSED.value(), keycode, 0, 0);
//						communicator.sendToQEMU(SendCommand.SEND_HARD_KEY_EVENT, keyEventData);
//					}
//
//					@Override
//					public void mouseUp(MouseEvent e) {
//						KeyEventData keyEventData = new KeyEventData(
//								KeyEventType.RELEASED.value(), keycode, 0, 0);
//						communicator.sendToQEMU(SendCommand.SEND_HARD_KEY_EVENT, keyEventData);
//					}
//
//					@Override
//					public void mouseDoubleClick(MouseEvent e) {
//						/* do nothing */
//					}
//				});
//			}
//
//			FormData dataComposite = new FormData();
//			dataComposite.left = new FormAttachment(displayCanvas, 0);
//			dataComposite.top = new FormAttachment(0, 0);
//			compositeBase.setLayoutData(dataComposite);
//		}
//	}

	@Override
	public void composerFinalize() {
		if (null != shellPaintListener) {
			shell.removePaintListener(shellPaintListener);
		}

		if (null != shellMouseMoveListener) {
			shell.removeMouseMoveListener(shellMouseMoveListener);
		}

		if (null != shellMouseListener) {
			shell.removeMouseListener(shellMouseListener);
		}

		if (toggleButton != null) {
			toggleButton.dispose();
		}

		if (skin.pairTagCanvas != null) {
			skin.pairTagCanvas.dispose();
		}

		if (backgroundColor != null) {
			backgroundColor.dispose();
		}

		frameMaker.freePatches();
	}
}
