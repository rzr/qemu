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
	protected Logger logger = SkinLogger.getSkinLogger(
			SkinWindow.class).getLogger();

	protected Shell shell;
	protected Shell parent;
	private int shellPositionType;

	public SkinWindow(Shell parent, int shellPositionType) {
		this.parent = parent;
		this.shellPositionType = shellPositionType;
	}

	public Shell getShell() {
		return shell;
	}

	public void open() {
		if (shell.isDisposed()) {
			return;
		}

		setShellPosition(shellPositionType);

		shell.open();

		while (!shell.isDisposed()) {
			if (!shell.getDisplay().readAndDispatch()) {
				shell.getDisplay().sleep();
			}
		}
	}

	protected void setShellPosition(int shellPositionType) {
		int x = 0;
		int y = 0;

		Rectangle monitorBounds = Display.getDefault().getBounds();
		logger.info("host monitor display bounds : " + monitorBounds);
		Rectangle parentBounds = parent.getBounds();
		logger.info("current parent shell bounds : " + parentBounds);
		Rectangle childBounds = shell.getBounds();
		logger.info("current child shell bounds : " + childBounds);

		if (shellPositionType == (SWT.RIGHT | SWT.TOP)) {
			x = parentBounds.x + parentBounds.width;
			y = parentBounds.y;

			/* correction of location */
			if ((x + childBounds.width) >
					(monitorBounds.x + monitorBounds.width)) {
				x = parentBounds.x - childBounds.width;
			}
		} else { /* SWT.RIGHT | SWT.CENTER */
			x = parentBounds.x + parentBounds.width;
			y = parentBounds.y + (parentBounds.height / 2) -
					(childBounds.height / 2);

			/* correction of location */
			int shift = (monitorBounds.x + monitorBounds.width) -
					(x + childBounds.width);
			if (shift < 0) {
				x += shift;
				parent.setLocation(parentBounds.x + shift, parentBounds.y);
			}
		}

		shell.setLocation(x, y);
	}
}
