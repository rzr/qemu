/**
 * Image Button
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

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.KeyAdapter;
import org.eclipse.swt.events.KeyEvent;
import org.eclipse.swt.events.MouseAdapter;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseMoveListener;
import org.eclipse.swt.events.MouseTrackAdapter;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Event;

public class CustomButton extends Canvas {
	/* state */
	private int mouse = 0;
	private boolean hit = false;

	/* image - 0 : normal, 1 : hover, 2 : pushed */
	private Image imageButton[];

	private int width;
	private int height;
	private String text;
	private int textPositionX;
	private int textPositionY;
	private static Color white;
	private static Color gray;

	public CustomButton(Composite parent, int style,
			Image imageNormal, Image imageHover, Image imagePushed) {
		super(parent, style);

		this.imageButton = new Image[3];
		imageButton[0] = imageNormal;
		imageButton[1] = imageHover;
		imageButton[2] = imagePushed;

		this.width = imageNormal.getImageData().width;
		this.height = imageNormal.getImageData().height;

		this.text = null;
		textPositionX = width / 2;
		textPositionY = height / 2;
		white = Display.getCurrent().getSystemColor(SWT.COLOR_WHITE);
		gray = Display.getCurrent().getSystemColor(SWT.COLOR_GRAY);

		this.addPaintListener(new PaintListener() {
			@Override
			public void paintControl(PaintEvent e) {
				switch (mouse) {
				case 0: /* default state */
					if (imageButton[0] != null) {
						e.gc.drawImage(imageButton[0], 0, 0);
					}

					if (text != null) {
						e.gc.setForeground(white);
						e.gc.drawText(text,
								textPositionX, textPositionY, true);
					}

					break;
				case 1: /* mouse over */
					if (imageButton[1] != null) {
						e.gc.drawImage(imageButton[1], 0, 0);
					}

					if (text != null) {
						e.gc.setForeground(white);
						e.gc.drawText(text,
								textPositionX, textPositionY, true);
					}

					break;
				case 2: /* mouse down */
					if (imageButton[2] != null) {
						e.gc.drawImage(imageButton[2], 0, 0);
					}

					if (text != null) {
						e.gc.setForeground(gray);
						e.gc.drawText(text,
								textPositionX, textPositionY, true);
					}

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
					/* dragging out */
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

	public void setText(String textValue) {
		/* checking whether the text value is appropriate */
		if (textValue == null || textValue.isEmpty()) {
			return;
		}

		GC gc = new GC(this);
		Point textSize = gc.textExtent(textValue);
		Point originalSize = textSize;

		while (textSize.x >= (width - 10)) {
			textValue = textValue.substring(0, textValue.length() - 1);
			textSize = gc.textExtent(textValue);
		}

		if (originalSize.x != textSize.x) {
			textValue = textValue.substring(0, textValue.length() - 1);
			textValue += "..";
			textSize = gc.textExtent(textValue);
		}

		gc.dispose();

		/* set text */
		text = textValue;

		textPositionX -= textSize.x / 2;
		if (textPositionX < 0) {
			textPositionX = 0;
		}

		textPositionY -= textSize.y / 2;
		if (textPositionY < 0) {
			textPositionY = 0;
		}
	}

	public String getText() {
		return text;
	}

	public Point getImageSize() {
		return new Point(imageButton[0].getImageData().width,
				imageButton[0].getImageData().height);
	}
}