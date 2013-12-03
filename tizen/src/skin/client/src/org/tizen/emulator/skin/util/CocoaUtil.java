/**
 *
 *
 * Copyright (C) 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * Munkyu Im <munkyu.im@samsung.com>
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

package org.tizen.emulator.skin.util;

import java.lang.reflect.InvocationTargetException;

public class CocoaUtil {

	private static final long aboutMenuValue = 0;
	private static final long prefMenuValue = 2;
	public static final String NSApplication_CLASS = "org.eclipse.swt.internal.cocoa.NSApplication";
	public static final String NSMenu_CLASS = "org.eclipse.swt.internal.cocoa.NSMenu";
	public static final String NSMenuItem_CLASS = "org.eclipse.swt.internal.cocoa.NSMenuItem";

	public static Object invokeMethod(Class<?> clazz, Object object, String method, Object[] args)
			throws IllegalArgumentException, IllegalAccessException, InvocationTargetException,
			SecurityException, NoSuchMethodException {
		Class<?>[] signature = new Class[args.length];
		for (int i = 0; i < args.length; i++) {
			Class<?> thisClass = args[i].getClass();
			if (thisClass == Integer.class)
				signature[i] = int.class;
			else if (thisClass == Long.class)
				signature[i] = long.class;
			else if (thisClass == Byte.class)
				signature[i] = byte.class;
			else if ( thisClass == Boolean.class )
                signature[i] = boolean.class;
			else
				signature[i] = thisClass;
		}
		return clazz.getDeclaredMethod(method, signature).invoke(object, args);
	}
	    
	// remove about and preference menu item
	public void removeTopMenuItems() {
		try {
			Class<?> nsmenuClass = Class.forName(NSMenu_CLASS);
			Class<?> nsmenuitemClass = Class.forName(NSMenuItem_CLASS);
			Class<?> nsapplicationClass = Class.forName(NSApplication_CLASS);
			
			Object sharedApplication = nsapplicationClass.getDeclaredMethod(
					"sharedApplication", (Class<?>[]) null).invoke(null,
					(Object[]) null);
			Object mainMenu = sharedApplication.getClass()
					.getDeclaredMethod("mainMenu", (Class<?>[]) null)
					.invoke(sharedApplication, (Object[]) null);

			Object mainMenuItem = invokeMethod(nsmenuClass, mainMenu,
					"itemAtIndex", new Object[] { new Long(0) });
			Object appMenu = mainMenuItem.getClass()
					.getDeclaredMethod("submenu", (Class<?>[]) null)
					.invoke(mainMenuItem, (Object[]) null);

			Object aboutMenuItem = invokeMethod(nsmenuClass, appMenu,
					"itemAtIndex", new Object[] { new Long(aboutMenuValue) });
			Object prefMenuItem = invokeMethod(nsmenuClass, appMenu,
					"itemAtIndex", new Object[] { new Long(prefMenuValue) });

			//set hidden
			invokeMethod(nsmenuitemClass, aboutMenuItem, "setHidden",
					new Object[] { new Boolean(true) });
			invokeMethod(nsmenuitemClass, prefMenuItem, "setHidden",
					new Object[] { new Boolean(true) });

		} catch (ClassNotFoundException e) {
			e.printStackTrace();
		} catch (IllegalArgumentException e) {
			e.printStackTrace();
		} catch (SecurityException e) {
			e.printStackTrace();
		} catch (IllegalAccessException e) {
			e.printStackTrace();
		} catch (InvocationTargetException e) {
			e.printStackTrace();
		} catch (NoSuchMethodException e) {
			e.printStackTrace();
		}
	}
}
