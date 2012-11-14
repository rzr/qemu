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
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Shell;
import org.tizen.emulator.skin.EmulatorSkinState;
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

	public PhoneShapeSkinComposer(EmulatorConfig config, Shell shell,
			EmulatorSkinState currentState, ImageRegistry imageRegistry) {
		this.config = config;
		this.shell = shell;
		this.currentState = currentState;
		this.imageRegistry = imageRegistry;
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

		if (/*skinInfo.isPhoneShape() &&*/ currentState.getCurrentImage() == null) {
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
}
