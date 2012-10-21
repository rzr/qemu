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

import java.util.logging.Logger;

import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Shell;
import org.tizen.emulator.skin.log.SkinLogger;

public class SkinWindow {
	private Logger logger = SkinLogger.getSkinLogger(
					SkinWindow.class).getLogger();

	protected Shell shell;
	protected Shell parent;

	public SkinWindow(Shell parent) {
		this.shell = new Shell(Display.getDefault(),
				SWT.CLOSE | SWT.TITLE | SWT.MIN | SWT.MAX);
		this.parent = parent;

		createContents();
	}

	public Shell getShell() {
		return shell;
	}

	public void open() {
		if (shell.isDisposed()) {
			return;
		}

		shell.pack();

		setShellSize();
		setShellPosition();

		shell.open();

		while (!shell.isDisposed()) {
			if (!shell.getDisplay().readAndDispatch()) {
				shell.getDisplay().sleep();
			}
		}
	}

	protected void createContents() {
	}

	protected void setShellSize() {
		shell.setSize(100, 300);
	}

	protected void setShellPosition() {
		Rectangle monitorBound = Display.getDefault().getBounds();
		logger.info("host monitor display bound : " + monitorBound);
		Rectangle emulatorBound = parent.getBounds();
		logger.info("current Emulator window bound : " + emulatorBound);
		Rectangle panelBound = shell.getBounds();
		logger.info("current Panel shell bound : " + panelBound);

		/* location correction */
		int x = emulatorBound.x + emulatorBound.width;
		int y = emulatorBound.y;
		if ((x + panelBound.width) > (monitorBound.x + monitorBound.width)) {
			x = emulatorBound.x - panelBound.width;
		}

		shell.setLocation(x, y);
	}
}
