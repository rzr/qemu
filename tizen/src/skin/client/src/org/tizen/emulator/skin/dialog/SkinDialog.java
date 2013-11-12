/**
 * Skin Dialog
 *
 * Copyright (C) 2011 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * GiWoong Kim <giwoong.kim@samsung.com>
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
 * HyunJun Son
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

import java.util.logging.Logger;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Dialog;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Shell;
import org.tizen.emulator.skin.log.SkinLogger;
import org.tizen.emulator.skin.util.StringUtil;

/**
 * 
 *
 */
public abstract class SkinDialog extends Dialog {
	public static final String OK = "        " + "OK" + "        ";

	private Logger logger =
			SkinLogger.getSkinLogger(SkinDialog.class).getLogger();

	protected Shell shell;
	protected Composite buttonComposite;
	private Shell parent;
	private int style;
	private String title;
	private boolean hasButton;

	/**
	 *  Constructor
	 */
	public SkinDialog(Shell parent, int style, String title) {
		super(parent, style);

		this.parent = parent;
		this.style = style;
		this.title = title;
		this.hasButton = true;
	}

	public SkinDialog(Shell parent, int style, String title, boolean hasButton) {
		super(parent, style);

		this.parent = parent;
		this.style = style;
		this.title = title;
		this.hasButton = hasButton;
	}

	protected void createComposite() {
		Composite parent = new Composite(shell, SWT.NONE);
		parent.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
		GridLayout gridLayout = new GridLayout(1, true);
		gridLayout.marginWidth = 20;
		parent.setLayout(gridLayout);

		Composite composite = new Composite(parent, SWT.NONE);
		composite.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
		composite.setLayout(new FillLayout(SWT.VERTICAL));

		Composite area = createArea(composite);
		if (null == area) {
			return;
		}

		if (hasButton == true) {
		/* bottom side */
			buttonComposite = new Composite(parent, SWT.NONE);
			buttonComposite.setLayoutData(new GridData(SWT.RIGHT, SWT.BOTTOM, true, true));
			buttonComposite.setLayout(new GridLayout(1, false));

			createButtons(buttonComposite);
		}
	}

	public void open() {
		shell = new Shell(parent, style);
		shell.setText(title);
		shell.setImage(parent.getImage());

		shell.setLayout(new GridLayout(1, true));

		createComposite();
		
		shell.pack();

		setShellSize();

		if (parent != null) {
			Point central = new Point(
					this.parent.getLocation().x + (this.parent.getSize().x / 2),
					this.parent.getLocation().y + (this.parent.getSize().y / 2));

			int width = shell.getSize().x;
			int height = shell.getSize().y;
			int x = central.x - (width / 2);
			int y = central.y - (height / 2);

			Rectangle monitorBounds = Display.getDefault().getBounds();

			if (x < monitorBounds.x) {
				x = monitorBounds.x;
			} else if ((x + width) > (monitorBounds.x + monitorBounds.width)) {
				x = (monitorBounds.x + monitorBounds.width) - width;
			}

			if (y < monitorBounds.y) {
				y = monitorBounds.y;
			} else if ((y + height) > (monitorBounds.y + monitorBounds.height)) {
				y = (monitorBounds.y + monitorBounds.height) - height;
			}

			shell.setLocation(x, y);
		}

		shell.open();

		while (!shell.isDisposed()) {
			if (!shell.getDisplay().readAndDispatch()) {
				shell.getDisplay().sleep();
			}
		}

		close();
	}

	protected void close() {
		logger.info("closed");

		/* do nothing */
	}

	protected void setShellSize() {
		/* do nothing */
	}

	protected abstract Composite createArea(Composite parent);

	protected void createButtons(Composite parent) {
		if (null == parent) {
			throw new IllegalArgumentException("button composite is null");
		}
	}

	protected Button createButton(Composite parent, String text) {
		if (null == parent) {
			throw new IllegalArgumentException("button composite is null");
		}

		Composite composite = new Composite(parent, SWT.NONE);
		GridLayout gridLayout = new GridLayout(1, true);
		composite.setLayout(gridLayout);

		Button button = new Button(composite, SWT.PUSH);
		GridData gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;

		button.setLayoutData(gd);
		button.setText(StringUtil.nvl(text));

		return button;
	}

	protected Button createOKButton(Composite parent, boolean setFocus) {
		logger.info("create OK button");

		Button okButton = createButton(parent, OK);
		okButton.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e) {
				shell.close();
			}
		});

		if (setFocus) {
			okButton.setFocus();
		}

		return okButton;
	}
}
