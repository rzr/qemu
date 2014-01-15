/**
 * Ramdump Dialog
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

package org.tizen.emulator.skin.dialog;

import java.io.IOException;
import java.util.logging.Level;
import java.util.logging.Logger;

import org.eclipse.swt.SWT;
import org.eclipse.swt.SWTException;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.graphics.ImageLoader;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.Shell;
import org.tizen.emulator.skin.comm.sock.SocketCommunicator;
import org.tizen.emulator.skin.config.EmulatorConfig;
import org.tizen.emulator.skin.log.SkinLogger;
import org.tizen.emulator.skin.util.SkinUtil;

public class RamdumpDialog extends SkinDialog {
	public static final String RAMDUMP_DIALOG_TITLE = "Ram Dump";
	public static final String INDICATOR_IMAGE_PATH = "images/process.gif";

	private Logger logger =
			SkinLogger.getSkinLogger(RamdumpDialog.class).getLogger();

	private SocketCommunicator communicator;
	private Composite compositeBase;
	private ImageData[] frames;

	/**
	 *  Constructor
	 */
	public RamdumpDialog(Shell parent,
			SocketCommunicator communicator, EmulatorConfig config) throws IOException {
		super(parent, SWT.DIALOG_TRIM | SWT.APPLICATION_MODAL,
				RAMDUMP_DIALOG_TITLE + " - " + SkinUtil.makeEmulatorName(config), false);

		this.communicator = communicator;
	}

	@Override
	protected Composite createArea(Composite parent) {
		Composite composite;

		try {
			composite = createContents(parent);
		} catch (IOException e) {
			logger.log(Level.SEVERE, e.getMessage(), e);
			return null;
		}

		shell.addListener(SWT.Close, new Listener() {
			@Override
			public void handleEvent(Event event) {
				if (communicator.getRamdumpFlag() == true) {
					/* do nothing */
					logger.info("do nothing");

					event.doit = false;
				}
			}
		});

		return composite;
	}

	private Composite createContents(Composite parent) throws IOException {
		compositeBase = new Composite(parent, SWT.NONE);
		compositeBase.setLayout(new GridLayout(2, false));

		final Display display = Display.getDefault();

		ClassLoader classLoader = this.getClass().getClassLoader();
		ImageLoader loader = new ImageLoader();

		try {
			frames = loader.load(
					classLoader.getResourceAsStream(INDICATOR_IMAGE_PATH));
		} catch (Exception e) {
			frames = null;
		}

		final Label labelImage = new Label(compositeBase, SWT.NONE);
		if (frames != null) {
			labelImage.setImage(new Image(display, frames[0]));
		}

		Label waitMsg = new Label(compositeBase, SWT.NONE);
		waitMsg.setText("     Please wait...");

		Thread animation = new Thread() {
			int currentFrame = 0;
			boolean isDisposed = false;

			@Override
			public void run() {
				while (!isDisposed) {
					try {
						if (frames != null) {
							sleep(50);
						} else {
							sleep(500);
						}
					} catch (InterruptedException e) {
						logger.warning("InterruptedException");
					}

					if (frames != null) {
						currentFrame = (currentFrame + 1) % frames.length;
					}
					if (display.isDisposed()) {
						return;
					}

					if (frames != null) {
						display.asyncExec(new Runnable() {
							@Override
							public void run() {
								try {
									Image newImage =
											new Image(display, frames[currentFrame]);
									labelImage.getImage().dispose();
									labelImage.setImage(newImage);
								} catch (SWTException e) {
									isDisposed = true;
								}
							}
						}); /* end of asyncExec */
					}

					if (communicator.getRamdumpFlag() == false) {
						isDisposed = true;
					}
				}

				display.syncExec(new Runnable() {
					@Override
					public void run() {
						logger.info("ramdump complete");

						if (labelImage.getImage() != null) {
							labelImage.getImage().dispose();
						}
						shell.setCursor(null);
						shell.close();
					}
				});
			}
		};

		shell.setCursor(display.getSystemCursor(SWT.CURSOR_WAIT));
		animation.start();

		return compositeBase;
	}

	@Override
	protected void setShellSize() {
		shell.setSize(280, 140);

		/* align */
		Rectangle boundsClient = shell.getClientArea();
		Rectangle boundsBase = compositeBase.getBounds();

		compositeBase.setBounds(
				(boundsClient.x + (boundsClient.width / 2)) - (boundsBase.width / 2),
				(boundsClient.y + (boundsClient.height / 2)) - (boundsBase.height / 2),
				boundsBase.width, boundsBase.height);
	}
}
