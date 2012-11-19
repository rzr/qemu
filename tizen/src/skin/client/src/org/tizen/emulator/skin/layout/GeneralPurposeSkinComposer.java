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

import java.util.List;
import java.util.logging.Logger;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Decorations;
import org.eclipse.swt.widgets.Shell;
import org.tizen.emulator.skin.EmulatorSkinState;
import org.tizen.emulator.skin.comm.ICommunicator.KeyEventType;
import org.tizen.emulator.skin.comm.ICommunicator.RotationInfo;
import org.tizen.emulator.skin.comm.ICommunicator.SendCommand;
import org.tizen.emulator.skin.comm.sock.SocketCommunicator;
import org.tizen.emulator.skin.comm.sock.data.KeyEventData;
import org.tizen.emulator.skin.config.EmulatorConfig;
import org.tizen.emulator.skin.config.EmulatorConfig.ArgsConstants;
import org.tizen.emulator.skin.config.EmulatorConfig.SkinPropertiesConstants;
import org.tizen.emulator.skin.dbi.KeyMapType;
import org.tizen.emulator.skin.dbi.RotationType;
import org.tizen.emulator.skin.image.ImageRegistry;
import org.tizen.emulator.skin.image.ImageRegistry.IconName;
import org.tizen.emulator.skin.log.SkinLogger;
import org.tizen.emulator.skin.util.SkinRotation;
import org.tizen.emulator.skin.util.SkinUtil;
import org.tizen.emulator.skin.util.SwtUtil;

public class GeneralPurposeSkinComposer implements ISkinComposer {
	private Logger logger = SkinLogger.getSkinLogger(
			GeneralPurposeSkinComposer.class).getLogger();

	private EmulatorConfig config;
	private Shell shell;
	private Canvas lcdCanvas;
	private Decorations decoration;
	private EmulatorSkinState currentState;

	private ImageRegistry imageRegistry;
	private SocketCommunicator communicator;

	public GeneralPurposeSkinComposer(EmulatorConfig config, Shell shell,
			EmulatorSkinState currentState, ImageRegistry imageRegistry,
			SocketCommunicator communicator) {
		this.config = config;
		this.shell = shell;
		this.decoration = null;
		this.currentState = currentState;
		this.imageRegistry = imageRegistry;
		this.communicator = communicator;
	}

	@Override
	public Canvas compose() {
		shell.setLayout(new FormLayout());

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
	}

	@Override
	public void arrangeSkin(int scale, short rotationId) {
		currentState.setCurrentScale(scale);
		currentState.setCurrentRotationId(rotationId);
		currentState.setCurrentAngle(SkinRotation.getAngle(rotationId));

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

		FormData dataCanvas = new FormData();
		dataCanvas.left = new FormAttachment(0, lcdBounds.x);
		dataCanvas.top = new FormAttachment(0, lcdBounds.y);
		dataCanvas.width = lcdBounds.width;
		dataCanvas.height = lcdBounds.height;
		lcdCanvas.setLayoutData(dataCanvas);

		if (decoration != null) {
			decoration.dispose();
			decoration = null;
		}

		shell.pack();

		List<KeyMapType> keyMapList =
				SkinUtil.getHWKeyMapList(currentState.getCurrentRotationId());

		if (keyMapList != null && keyMapList.isEmpty() == false) {
			decoration = new Decorations(shell, SWT.BORDER);
			decoration.setLayout(new GridLayout(1, true));

			for (KeyMapType keyEntry : keyMapList) {
				Button hardKeyButton = new Button(decoration, SWT.FLAT);
				hardKeyButton.setText(keyEntry.getEventInfo().getKeyName());
				hardKeyButton.setToolTipText(keyEntry.getTooltip());

				hardKeyButton.setLayoutData(new GridData(SWT.FILL,	SWT.FILL, true, false));

				final int keycode = keyEntry.getEventInfo().getKeyCode();
				hardKeyButton.addMouseListener(new MouseListener() {
					@Override
					public void mouseDown(MouseEvent e) {
						KeyEventData keyEventData = new KeyEventData(
								KeyEventType.PRESSED.value(), keycode, 0, 0);
						communicator.sendToQEMU(SendCommand.SEND_HARD_KEY_EVENT, keyEventData);
					}

					@Override
					public void mouseUp(MouseEvent e) {
						KeyEventData keyEventData = new KeyEventData(
								KeyEventType.RELEASED.value(), keycode, 0, 0);
						communicator.sendToQEMU(SendCommand.SEND_HARD_KEY_EVENT, keyEventData);
					}

					@Override
					public void mouseDoubleClick(MouseEvent e) {
						/* do nothing */
					}
				});
			}

			FormData dataDecoration = new FormData();
			dataDecoration.left = new FormAttachment(lcdCanvas, 0);
			dataDecoration.top = new FormAttachment(0, 0);
			decoration.setLayoutData(dataDecoration);
		}

		shell.redraw();
		shell.pack();
	}

	@Override
	public Rectangle adjustLcdGeometry(
			Canvas lcdCanvas, int resolutionW, int resolutionH,
			int scale, short rotationId) {
		Rectangle lcdBounds = new Rectangle(0, 0, 0, 0);

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

	@Override
	public void composerFinalize() {
		/* do nothing */
	}
}
