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
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
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
	private Image imageFrame;
	private SocketCommunicator communicator;
	private List<KeyMapType> keyMapList;

	private PaintListener shellPaintListener;
	private MouseMoveListener shellMouseMoveListener;
	private MouseListener shellMouseListener;

	private boolean isGrabbedShell;
	private Point grabPosition;

	public ControlPanel(Shell parent,
			SocketCommunicator communicator, List<KeyMapType> keyMapList) {
		super(parent);

		this.shell = new Shell(Display.getDefault(), SWT.NO_TRIM);
		this.frameMaker = new SkinPatches(PATCH_IMAGES_PATH); //TODO: freePatches
		this.imageFrame = frameMaker.getPatchedImage(136, 140);

		this.keyMapList = keyMapList;
		this.communicator = communicator;
		this.grabPosition = new Point(0, 0);

		createContents();
		addControlPanelListener(); //TODO: remove

		shell.setSize((frameMaker.getPatchWidth() * 2) + 136,
				(frameMaker.getPatchHeight() * 2) + 140);
	}

	protected void createContents() {
		GridLayout gridLayout = new GridLayout(1, true);
		gridLayout.marginLeft = gridLayout.marginRight = frameMaker.getPatchWidth() + 6;
		gridLayout.marginTop = frameMaker.getPatchHeight() + 20;
		gridLayout.marginBottom = frameMaker.getPatchHeight() + 6;
		gridLayout.marginWidth = gridLayout.marginHeight = 0;
		gridLayout.horizontalSpacing = gridLayout.verticalSpacing = 0;

		shell.setLayout(gridLayout);

		ScrolledComposite compositeScroll = new ScrolledComposite(shell, SWT.V_SCROLL);
		compositeScroll.setLayoutData(new GridData(SWT.LEFT, SWT.TOP, true, true, 1, 1));
		Composite compositeBase = new Composite(compositeScroll, SWT.NONE);
		compositeBase.setLayout(gridLayout);

		if (keyMapList != null && keyMapList.isEmpty() == false) {
			for (KeyMapType keyEntry : keyMapList) {
				Button HWKeyButton = new Button(compositeBase, SWT.FLAT);
				HWKeyButton.setText(keyEntry.getEventInfo().getKeyName());
				HWKeyButton.setToolTipText(keyEntry.getTooltip());
				HWKeyButton.setSize(102, 28);
				HWKeyButton.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false));

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
	}
}
