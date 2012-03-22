/**
 * 
 *
 * Copyright ( C ) 2011 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * Hyunjun Son <hj79.son@samsung.com>
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

package org.tizen.emulator.skin.screenshot;

import java.io.FileOutputStream;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.Date;

import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.ScrolledComposite;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.graphics.ImageLoader;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.MessageBox;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.ToolBar;
import org.eclipse.swt.widgets.ToolItem;
import org.tizen.emulator.skin.util.IOUtil;

public class ScreenShotDialog {
	private Image image;
	private Shell dialog;
	private Shell shell;

	private Canvas canvas;
	private ToolBar bar ;

	public ScreenShotDialog( Shell shell, Canvas lcdCanvas ) {
		this.shell = shell;
		// image copy
		this.image = new Image( shell.getDisplay(), lcdCanvas.getBounds() );
		GC gc = null;
		try {
			gc = new GC( lcdCanvas );
			gc.copyArea(image, 0, 0);
		} finally {
			if ( null != gc ) {
				gc.dispose();
			}
		}

		dialog = new Shell( shell, SWT.DIALOG_TRIM );
		dialog.setLayout(new FormLayout());
		dialog.setText( "Screen Shot" );
		dialog.addListener( SWT.Close, new Listener() {
			@Override
			public void handleEvent( Event event ) {
					image.dispose();
				}
		});

		// tool bar
		FormData barData = new FormData();
		barData.left  = new FormAttachment( 0 );
		barData.right = new FormAttachment( 100 );
		barData.top   = new FormAttachment( 0 );
		bar = new ToolBar( dialog, SWT.HORIZONTAL );
		bar.setLayoutData( barData );
		makeToolBarMenu();

		// dummy for bar.getSize() 
		dialog.setSize( lcdCanvas.getSize().x, lcdCanvas.getSize().y + bar.getSize().y + 1 );

		dialog.setSize( lcdCanvas.getSize().x, lcdCanvas.getSize().y + bar.getSize().y + 29 /* Title bar */ );
		dialog.update();

		// canvas
		ScrolledComposite composite = new ScrolledComposite( dialog, SWT.V_SCROLL | SWT.H_SCROLL );
		canvas = new Canvas( composite, SWT.NONE );
		composite.setContent( canvas );
		Rectangle rect = image.getBounds();
		rect = new Rectangle( rect.x, rect.y, rect.width, rect.height + bar.getSize().y );
		canvas.setBounds(rect);
		canvas.addPaintListener(  new PaintListener() {
			@Override
			public void paintControl( PaintEvent e ) {
				e.gc.drawImage(image, 0, bar.getSize().y);
			}
		});
	}

	private ToolItem saveToolItem;
	private void makeToolBarMenu() {
		saveToolItem = new ToolItem( bar, SWT.PUSH );
		saveToolItem.setText( "Save" );
		saveToolItem.setToolTipText( "Save image file" );
		saveToolItem.addSelectionListener( new SelectionListener() {
			@Override
			public void widgetSelected( SelectionEvent e ) {
				FileDialog fd = new FileDialog( dialog, SWT.SAVE );
				fd.setText( "Save Image... " );
				String[] filter = { "*.png;*.PNG;*.jpg;*.JPG;*.jpeg;*.JPEG;*.bmp;*.BMP" };
				String[] filterName = { "Image Files" };
				fd.setFilterExtensions( filter );
				fd.setFilterNames( filterName );

				SimpleDateFormat formatter = new SimpleDateFormat( "yyyy-mm-dd-hhmmss" );
				fd.setFileName( "default-" + formatter.format( new Date(System.currentTimeMillis() ) ) + ".png" );

				String path = fd.open();
				if ( path == null ) {
					return;
				}

				int i = path.lastIndexOf( '.' );
				String filePath = path;
				String format = null;
				if (i != -1) {
					filePath = path.substring( 0, i-1 );
					format = path.substring(i+1, path.length());
					format = format.toLowerCase();
				}

				FileOutputStream fos = null;
				try {
					fos = new FileOutputStream( filePath + "." + format, false );
					ImageLoader loader = new ImageLoader();
					loader.data = new ImageData[] { image.getImageData() };
					if (format == null || format.equals( "png" )) {
						loader.save( fos, SWT.IMAGE_PNG );
					} else if ( format.equals( "jpg" ) || format.equals( "jpeg" ) ) {
						loader.save( fos, SWT.IMAGE_JPEG );
					} else if (format.equals( "bmp" )) {
						loader.save( fos, SWT.IMAGE_BMP );
					}  else {
						MessageBox msg = new MessageBox( dialog,  SWT.ICON_WARNING );
						msg.setText( "Warning" );
						msg.setMessage( "You must add the extension of file. (PNG/JPG/BMP)" );
						msg.open();
					}
				} catch ( IOException ex )  {
					ex.printStackTrace();
				} finally {
					IOUtil.close( fos );
				}
			}
			@Override
			public void widgetDefaultSelected( SelectionEvent e ) {
			}
		} );
	}

	private static int count = 0;
	public void open() {
		dialog.setLocation( shell.getLocation().x + dialog.getSize().x + ( ++count % 5 ) * 20,
							shell.getLocation().y + ( count % 5 ) * 20 );
		dialog.open();
	}
}