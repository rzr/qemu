/**
 * General Key Window
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

package org.tizen.emulator.skin.custom;

import java.util.List;

import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.ScrolledComposite;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.events.MouseMoveListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.events.ShellEvent;
import org.eclipse.swt.events.ShellListener;
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
import org.eclipse.swt.widgets.Shell;
import org.tizen.emulator.skin.EmulatorSkin;
import org.tizen.emulator.skin.comm.ICommunicator.KeyEventType;
import org.tizen.emulator.skin.comm.ICommunicator.SendCommand;
import org.tizen.emulator.skin.comm.sock.data.KeyEventData;
import org.tizen.emulator.skin.dbi.KeyMapType;
import org.tizen.emulator.skin.image.GeneralKeyWindowImageRegistry;
import org.tizen.emulator.skin.image.GeneralKeyWindowImageRegistry.GeneralKeyWindowImageName;
import org.tizen.emulator.skin.layout.SkinPatches;
import org.tizen.emulator.skin.util.SwtUtil;

public class GeneralKeyWindow extends SkinWindow {
	private static final int SHELL_MARGIN_BOTTOM = 3;
	private static final int PAIRTAG_CIRCLE_SIZE = 8;
	private static final int PAIRTAG_MARGIN_BOTTOM = 6;
	private static final int BUTTON_DEFAULT_CNT = 4;
	private static final int BUTTON_VERTICAL_SPACING = 7;
	private static final int SCROLLBAR_HORIZONTAL_SPACING = 4;
	private static final int SCROLLBAR_SIZE_WIDTH = 14;

	private EmulatorSkin skin;
	private SkinPatches frameMaker;

	private int widthBase;
	private int heightBase;
	private int widthScrollbar;
	private int cntHiddenButton;

	private Image imageNormal; /* ImageButton image */
	private Image imageHover; /* hovered ImageButton image */
	private Image imagePushed; /* pushed ImageButton image */
	private Image imageFrame; /* nine-patch image */

	private Color colorFrame;
	private GeneralKeyWindowImageRegistry imageRegistry;
	private List<KeyMapType> keyMapList;

	private ShellListener shellListener;
	private PaintListener shellPaintListener;
	private MouseMoveListener shellMouseMoveListener;
	private MouseListener shellMouseListener;

	private boolean isGrabbedShell;
	private Point grabPosition;

	public GeneralKeyWindow(EmulatorSkin skin,
			GeneralKeyWindowImageRegistry imageRegstry, List<KeyMapType> keyMapList) {
		super(skin.getShell(), SWT.RIGHT | SWT.CENTER);

		this.skin = skin;
		this.parent = skin.getShell();
		this.shell = new Shell(parent.getDisplay() /* for Mac & Always on Top */,
				SWT.NO_TRIM | SWT.RESIZE | SWT.TOOL | SWT.NO_FOCUS);

		this.imageRegistry = imageRegstry;
		this.frameMaker = new SkinPatches(
				imageRegistry.getKeyWindowImage(
						GeneralKeyWindowImageName.KEYWINDOW_PATCH_LT),
				imageRegistry.getKeyWindowImage(
						GeneralKeyWindowImageName.KEYWINDOW_PATCH_T),
				imageRegistry.getKeyWindowImage(
						GeneralKeyWindowImageName.KEYWINDOW_PATCH_RT),
				imageRegistry.getKeyWindowImage(
						GeneralKeyWindowImageName.KEYWINDOW_PATCH_L),
				imageRegistry.getKeyWindowImage(
						GeneralKeyWindowImageName.KEYWINDOW_PATCH_R),
				imageRegistry.getKeyWindowImage(
						GeneralKeyWindowImageName.KEYWINDOW_PATCH_LB),
				imageRegistry.getKeyWindowImage(
						GeneralKeyWindowImageName.KEYWINDOW_PATCH_B),
				imageRegistry.getKeyWindowImage(
						GeneralKeyWindowImageName.KEYWINDOW_PATCH_RB));

		this.keyMapList = keyMapList; //TODO: null
		this.grabPosition = new Point(0, 0);

		shell.setText(parent.getText());
		shell.setImage(parent.getImage());

		/* load image for HW key button */
		imageNormal = imageRegistry.getKeyWindowImage(
				GeneralKeyWindowImageName.KEYBUTTON_NORMAL);
		imageHover = imageRegistry.getKeyWindowImage(
				GeneralKeyWindowImageName.KEYBUTTON_HOVER);
		imagePushed = imageRegistry.getKeyWindowImage(
				GeneralKeyWindowImageName.KEYBUTTON_PUSHED);

		/* calculate the key window size */
		widthBase = imageNormal.getImageData().width;
		heightBase = (imageNormal.getImageData().height * BUTTON_DEFAULT_CNT) +
				(BUTTON_VERTICAL_SPACING * (BUTTON_DEFAULT_CNT - 1));

		widthScrollbar = SCROLLBAR_SIZE_WIDTH + SCROLLBAR_HORIZONTAL_SPACING;
		int heightHeaderPart = (PAIRTAG_CIRCLE_SIZE + PAIRTAG_MARGIN_BOTTOM);
		int heightTailPart = SHELL_MARGIN_BOTTOM;

		/* make a frame image */
		if (keyMapList != null) {
			this.cntHiddenButton = keyMapList.size() - BUTTON_DEFAULT_CNT;
		}

		this.imageFrame = frameMaker.getPatchedImage(
				widthBase + ((cntHiddenButton > 0) ? widthScrollbar : 0),
				heightBase + heightHeaderPart + heightTailPart);
		this.colorFrame = new Color(shell.getDisplay(), new RGB(38, 38, 38));

		shell.setBackground(colorFrame);

		createContents();
		trimPatchedShell(shell, imageFrame);

		addKeyWindowListener();

		shell.setSize(imageFrame.getImageData().width,
				imageFrame.getImageData().height);
	}

	protected void createContents() {
		GridLayout shellGridLayout = new GridLayout(1, false);
		shellGridLayout.marginLeft = shellGridLayout.marginRight = frameMaker.getPatchWidth();
		shellGridLayout.marginTop = frameMaker.getPatchHeight();
		shellGridLayout.marginBottom = frameMaker.getPatchHeight() + SHELL_MARGIN_BOTTOM;
		shellGridLayout.marginWidth = shellGridLayout.marginHeight = 0;
		shellGridLayout.horizontalSpacing = shellGridLayout.verticalSpacing = 0;

		shell.setLayout(shellGridLayout);

		/* make a pair tag circle */
		ColorTag pairTagCanvas = new ColorTag(shell, SWT.NONE, skin.getColorVM());
		pairTagCanvas.setLayoutData(new GridData(PAIRTAG_CIRCLE_SIZE,
				PAIRTAG_CIRCLE_SIZE + PAIRTAG_MARGIN_BOTTOM));

		/* make a region of HW keys */
		if (cntHiddenButton > 0) {
			/* added custom scrollbar */
			Image imagesScrollArrowUp[] = new Image[3];
			Image imagesScrollArrowDown[] = new Image[3];

			imagesScrollArrowUp[0] = imageRegistry.getKeyWindowImage(
					GeneralKeyWindowImageName.SCROLL_UPBUTTON_NORMAL);
			imagesScrollArrowUp[1] = imageRegistry.getKeyWindowImage(
					GeneralKeyWindowImageName.SCROLL_UPBUTTON_HOVER);
			imagesScrollArrowUp[2] = imageRegistry.getKeyWindowImage(
					GeneralKeyWindowImageName.SCROLL_UPBUTTON_PUSHED);

			imagesScrollArrowDown[0] = imageRegistry.getKeyWindowImage(
					GeneralKeyWindowImageName.SCROLL_DOWNBUTTON_NORMAL);
			imagesScrollArrowDown[1] = imageRegistry.getKeyWindowImage(
					GeneralKeyWindowImageName.SCROLL_DOWNBUTTON_HOVER);
			imagesScrollArrowDown[2] = imageRegistry.getKeyWindowImage(
					GeneralKeyWindowImageName.SCROLL_DOWNBUTTON_PUSHED);

			CustomScrolledComposite compositeScroll =
					new CustomScrolledComposite(shell, SWT.NONE,
							imagesScrollArrowUp, imagesScrollArrowDown,
							imageRegistry.getKeyWindowImage(
									GeneralKeyWindowImageName.SCROLL_THUMB),
							imageRegistry.getKeyWindowImage(
									GeneralKeyWindowImageName.SCROLL_SHAFT));
			compositeScroll.setLayoutData(new GridData(SWT.LEFT, SWT.TOP, true, true, 1, 1));

			Composite compositeBase = new Composite(compositeScroll, SWT.NONE);

			createHwKeys(compositeBase);

			Point sizeContent = compositeBase.computeSize(
					widthBase + widthScrollbar, heightBase);
			compositeScroll.setContent(compositeBase, sizeContent);
			compositeScroll.setExpandHorizontal(true);
			compositeScroll.setExpandVertical(true);

			sizeContent.y += (imageNormal.getImageData().height * cntHiddenButton) +
					(BUTTON_VERTICAL_SPACING * cntHiddenButton);
			compositeScroll.setMinSize(sizeContent);
		} else {
			ScrolledComposite compositeScroll = new ScrolledComposite(shell, SWT.V_SCROLL);
			compositeScroll.setLayoutData(new GridData(SWT.LEFT, SWT.TOP, true, true, 1, 1));

			Composite compositeBase = new Composite(compositeScroll, SWT.NONE);
			createHwKeys(compositeBase);

			compositeScroll.setContent(compositeBase);
			compositeScroll.setExpandHorizontal(true);
			compositeScroll.setExpandVertical(true);

			compositeScroll.setMinSize(
					compositeBase.computeSize(SWT.DEFAULT, SWT.DEFAULT));
		}
	}

	protected void createHwKeys(Composite composite) {
		composite.setBackground(colorFrame);

		GridLayout compositeGridLayout = new GridLayout(1, false);
		compositeGridLayout.marginLeft = compositeGridLayout.marginRight = 0;
		compositeGridLayout.marginTop = compositeGridLayout.marginBottom = 0;
		compositeGridLayout.marginWidth = compositeGridLayout.marginHeight = 0;
		compositeGridLayout.horizontalSpacing = 0;
		compositeGridLayout.verticalSpacing = BUTTON_VERTICAL_SPACING;
		composite.setLayout(compositeGridLayout);

		/* attach HW keys */
		if (keyMapList != null && keyMapList.isEmpty() == false) {
			for (KeyMapType keyEntry : keyMapList) {
				final CustomButton HWKeyButton = new CustomButton(composite,
						SWT.NO_FOCUS, imageNormal, imageHover, imagePushed);
				HWKeyButton.setText(keyEntry.getEventInfo().getKeyName());
				HWKeyButton.setToolTipText(keyEntry.getTooltip());
				HWKeyButton.setBackground(colorFrame);
				HWKeyButton.setLayoutData(new GridData(imageNormal.getImageData().width,
								imageNormal.getImageData().height));

				final int keycode = keyEntry.getEventInfo().getKeyCode();
				HWKeyButton.addMouseListener(new MouseListener() {
					@Override
					public void mouseDown(MouseEvent e) {
						logger.info(HWKeyButton.getText() + " key is pressed");

						KeyEventData keyEventData = new KeyEventData(
								KeyEventType.PRESSED.value(), keycode, 0, 0);
						skin.communicator.sendToQEMU(
								SendCommand.SEND_HARD_KEY_EVENT, keyEventData, false);
					}

					@Override
					public void mouseUp(MouseEvent e) {
						logger.info(HWKeyButton.getText() + " key is released");

						KeyEventData keyEventData = new KeyEventData(
								KeyEventType.RELEASED.value(), keycode, 0, 0);
						skin.communicator.sendToQEMU(
								SendCommand.SEND_HARD_KEY_EVENT, keyEventData, false);
					}

					@Override
					public void mouseDoubleClick(MouseEvent e) {
						/* do nothing */
					}
				});
			}
		}
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
		int colorKey;

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

	private void addKeyWindowListener() {
		shellPaintListener = new PaintListener() {
			@Override
			public void paintControl(final PaintEvent e) {
				if (imageFrame != null) {
					e.gc.drawImage(imageFrame, 0, 0);
				}
			}
		};

		shell.addPaintListener(shellPaintListener);

		shellListener = new ShellListener() {
			@Override
			public void shellClosed(ShellEvent event) {
				logger.info("General Key Window is closed");

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
				if (isGrabbedShell == true && e.button == 0/* left button */) {
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
	}
}
