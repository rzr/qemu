/**
 *
 *
 * Copyright ( C ) 2011 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * GiWoong Kim <giwoong.kim@samsung.com>
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or ( at your option ) any later version.
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

package org.tizen.emulator.skin.window;

import java.util.List;

import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.ScrolledComposite;
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
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.Shell;
import org.tizen.emulator.skin.comm.ICommunicator.KeyEventType;
import org.tizen.emulator.skin.comm.ICommunicator.SendCommand;
import org.tizen.emulator.skin.comm.sock.SocketCommunicator;
import org.tizen.emulator.skin.comm.sock.data.KeyEventData;
import org.tizen.emulator.skin.dbi.KeyMapType;
import org.tizen.emulator.skin.layout.SkinPatches;

public class ControlPanel extends SkinWindow {
	private static final String PATCH_IMAGES_PATH = "images/key-window/";

	private SkinPatches frameMaker;
	private Image imageNormal; /* ImageButton image */
	private Image imageHover; /* hovered ImageButton image */
	private Image imagePushed; /* pushed ImageButton image */
	private Image imageFrame; /* nine-patch image */
	private Color colorFrame;
	private SocketCommunicator communicator;
	private List<KeyMapType> keyMapList;

	private PaintListener shellPaintListener;
	private MouseMoveListener shellMouseMoveListener;
	private MouseListener shellMouseListener;
	private Listener shellCloseListener;

	private boolean isGrabbedShell;
	private Point grabPosition;

	public ControlPanel(Shell parent,
			SocketCommunicator communicator, List<KeyMapType> keyMapList) {
		super(parent);

		this.shell = new Shell(Display.getDefault(), SWT.NO_TRIM | SWT.RESIZE);
		this.frameMaker = new SkinPatches(PATCH_IMAGES_PATH);

		/* load image for ImageButton */
		ClassLoader loader = this.getClass().getClassLoader();
		imageNormal = new Image(Display.getDefault(),
				loader.getResourceAsStream(PATCH_IMAGES_PATH + "keybutton_nml.png"));
		imageHover = new Image(Display.getDefault(),
						loader.getResourceAsStream(PATCH_IMAGES_PATH + "keybutton_hover.png"));
		imagePushed = new Image(Display.getDefault(),
						loader.getResourceAsStream(PATCH_IMAGES_PATH + "keybutton_pushed.png"));

		/* calculate the key window size */
		int width = imageNormal.getImageData().width;
		int height = (imageNormal.getImageData().height * 4) + (4 * 3);

		/* make a frame image */
		this.imageFrame = frameMaker.getPatchedImage(width, height);
		this.colorFrame = new Color(shell.getDisplay(), new RGB(38, 38, 38));

		this.keyMapList = keyMapList;
		this.communicator = communicator;
		this.grabPosition = new Point(0, 0);

		createContents();
		trimPatchedShell(shell, imageFrame);
		addControlPanelListener();

		shell.setBackground(colorFrame);
		shell.setSize(imageFrame.getImageData().width,
				imageFrame.getImageData().height);
	}

	protected void createContents() {
		GridLayout shellGridLayout = new GridLayout(1, true);
		shellGridLayout.marginLeft = shellGridLayout.marginRight = frameMaker.getPatchWidth();
		shellGridLayout.marginTop = shellGridLayout.marginBottom = frameMaker.getPatchHeight();
		shellGridLayout.marginWidth = shellGridLayout.marginHeight = 0;
		shellGridLayout.horizontalSpacing = shellGridLayout.verticalSpacing = 0;

		shell.setLayout(shellGridLayout);

		ScrolledComposite compositeScroll = new ScrolledComposite(shell, SWT.V_SCROLL);
		compositeScroll.setLayoutData(new GridData(SWT.LEFT, SWT.TOP, true, true, 1, 1));
		Composite compositeBase = new Composite(compositeScroll, SWT.NONE);
		//compositeBase.setBackground(colorFrame);

		GridLayout compositeGridLayout = new GridLayout(1, false);
		compositeGridLayout.marginLeft = compositeGridLayout.marginRight = 0;
		compositeGridLayout.marginTop = compositeGridLayout.marginBottom = 0;
		compositeGridLayout.marginWidth = compositeGridLayout.marginHeight = 0;
		compositeGridLayout.horizontalSpacing = 0;
		compositeGridLayout.verticalSpacing = 4;
		compositeBase.setLayout(compositeGridLayout);

		if (keyMapList != null && keyMapList.isEmpty() == false) {
			for (KeyMapType keyEntry : keyMapList) {
				ImageButton HWKeyButton = new ImageButton(compositeBase, SWT.NONE,
						imageNormal, imageHover, imagePushed);
				//HWKeyButton.setText(keyEntry.getEventInfo().getKeyName());
				//HWKeyButton.setToolTipText(keyEntry.getTooltip());
				HWKeyButton.setBackground(colorFrame);
				HWKeyButton.setLayoutData(new GridData(imageNormal.getImageData().width,
								imageNormal.getImageData().height));

				final int keycode = keyEntry.getEventInfo().getKeyCode();
				HWKeyButton.addMouseListener(new MouseListener() {
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
		}

		compositeScroll.setContent(compositeBase);
		compositeScroll.setExpandHorizontal(true);
		compositeScroll.setExpandVertical(true);
		compositeScroll.setMinSize(compositeBase.computeSize(SWT.DEFAULT, SWT.DEFAULT));
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

		for (int i = 0; i < width; i++) {
			for (int j = 0; j < height; j++) {
				int colorPixel = imageData.getPixel(i, j);
				if (colorPixel == 0xFF00FF /* magenta */) {
					region.subtract(i, j, 1, 1);
				}
			}
		}

		shell.setRegion(region);
	}

	private void addControlPanelListener() {
		shellPaintListener = new PaintListener() {
			@Override
			public void paintControl(final PaintEvent e) {
				if (imageFrame != null) {
					e.gc.drawImage(imageFrame, 0, 0);
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
				}
			}

			@Override
			public void mouseDown(MouseEvent e) {
				if (1 == e.button) { /* left button */
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

		shellCloseListener = new Listener() {
			@Override
			public void handleEvent(Event event) {
				logger.info("Key Window is closed");

				if (null != shellPaintListener) {
					shell.removePaintListener(shellPaintListener);
				}

				if (null != shellMouseMoveListener) {
					shell.removeMouseMoveListener(shellMouseMoveListener);
				}

				if (null != shellMouseListener) {
					shell.removeMouseListener(shellMouseListener);
				}

				frameMaker.freePatches();
			}
		};

		shell.addListener(SWT.Close, shellCloseListener);
	}
}
