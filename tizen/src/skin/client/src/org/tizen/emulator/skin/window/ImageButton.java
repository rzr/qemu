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

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.KeyAdapter;
import org.eclipse.swt.events.KeyEvent;
import org.eclipse.swt.events.MouseAdapter;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseMoveListener;
import org.eclipse.swt.events.MouseTrackAdapter;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Event;

public class ImageButton extends Canvas {
	private int mouse = 0;
	private boolean hit = false;

	public ImageButton(Composite parent, int style) {
		super(parent, style);

		this.addPaintListener(new PaintListener() {
			@Override
			public void paintControl(PaintEvent e) {
				switch (mouse) {
				case 0:
					/* default state */
					e.gc.drawString("Normal", 5, 5);
					break;
				case 1:
					/* mouse over */
					e.gc.drawString("Mouse over", 5, 5);
					break;
				case 2:
					/* mouse down */
					e.gc.drawString("Hit", 5, 5);
					break;
				default:
					break;
				}
			}
		});

		this.addMouseMoveListener(new MouseMoveListener() {
			@Override
			public void mouseMove(MouseEvent e) {
				if (!hit) {
					return;
				}

				mouse = 2;

				if (e.x < 0 || e.y < 0 ||
						e.x > getBounds().width || e.y > getBounds().height) {
					mouse = 0;
				}

				redraw();
			}
		});

		this.addMouseTrackListener(new MouseTrackAdapter() {
			@Override
			public void mouseEnter(MouseEvent e) {
				mouse = 1;

				redraw();
			}

			@Override
			public void mouseExit(MouseEvent e) {
				mouse = 0;

				redraw();
			}
		});

		this.addMouseListener(new MouseAdapter() {
			@Override
			public void mouseDown(MouseEvent e) {
				hit = true;
				mouse = 2;

				redraw();
			}

			@Override
			public void mouseUp(MouseEvent e) {
				hit = false;
				mouse = 1;
			
				if (e.x < 0 || e.y < 0 ||
						e.x > getBounds().width || e.y > getBounds().height) {
					mouse = 0;
				}
			
				redraw();
			
				if (mouse == 1) {
					notifyListeners(SWT.Selection, new Event());
				}
			}
		});

		this.addKeyListener(new KeyAdapter() {
			@Override
			public void keyPressed(KeyEvent e) {
				if (e.keyCode == '\r' || e.character == ' ') {
					Event event = new Event();
					notifyListeners(SWT.Selection, event);
				}
			}
		});
	}

}