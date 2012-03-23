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
import java.util.logging.Level;
import java.util.logging.Logger;

import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.ScrolledComposite;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.graphics.ImageLoader;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Dialog;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.MenuItem;
import org.eclipse.swt.widgets.MessageBox;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.ToolBar;
import org.tizen.emulator.skin.config.EmulatorConfig;
import org.tizen.emulator.skin.log.SkinLogger;
import org.tizen.emulator.skin.util.IOUtil;
import org.tizen.emulator.skin.util.SkinUtil;

public class ScreenShotDialog extends Dialog {

	private Logger logger = SkinLogger.getSkinLogger( ScreenShotDialog.class ).getLogger();

	private Image image;
	private Shell shell;
	private Shell parent;
	private EmulatorConfig config;
	
	private Canvas canvas;

	public ScreenShotDialog( Shell parent, Canvas lcdCanvas, EmulatorConfig config ) {
		super( parent );

		this.parent = parent;
		this.config = config;
		
		// image copy
		capture( lcdCanvas );

		shell = new Shell( parent, SWT.DIALOG_TRIM );
		shell.setLayout( new FormLayout() );
		shell.setText( "Screen Shot" );
		shell.setLocation( parent.getLocation().x + parent.getSize().x + 50, parent.getLocation().y );
		shell.addListener( SWT.Close, new Listener() {
			@Override
			public void handleEvent( Event event ) {
				image.dispose();
			}
		} );
		
		Menu menu = new Menu( shell, SWT.BAR );
		makeToolBarMenu( menu );
		shell.setMenuBar( menu );

		// canvas
		ScrolledComposite composite = new ScrolledComposite( shell, SWT.V_SCROLL | SWT.H_SCROLL );
		
		canvas = new Canvas( composite, SWT.NONE );
		composite.setContent( canvas );
		
		ImageData imageData = image.getImageData();
		Rectangle rect = new Rectangle( 0, 0, imageData.width, imageData.height );
		canvas.setBounds( rect );
		
		canvas.addPaintListener( new PaintListener() {
			@Override
			public void paintControl( PaintEvent e ) {
				e.gc.drawImage( image, 0, 0 );
			}
		} );

		int width = 0;
		int height = 0;

		Point shellLocation = shell.getLocation();
		Rectangle monitorSize = shell.getDisplay().getClientArea();

		if( shellLocation.y + imageData.height > monitorSize.height ) {
			shell.setSize( imageData.width, imageData.height );
		}else {
			shell.pack();
		}
		
	}

	private void capture( Canvas canvas ) {
		
		if( null != this.image ) {
			this.image.dispose();
		}
		
		this.image = new Image( parent.getDisplay(), canvas.getBounds() );
		
		GC gc = null;
		try {
			gc = new GC( canvas );
			gc.copyArea( image, 0, 0 );
		} finally {
			if ( null != gc ) {
				gc.dispose();
			}
		}
		
		canvas.update();
		
	}
	
	private void makeToolBarMenu( Menu menu ) {
		
		MenuItem saveItem = new MenuItem( menu, SWT.FLAT );
		saveItem.setText( "Save" );
		
		saveItem.addSelectionListener( new SelectionAdapter() {
			
			@Override
			public void widgetSelected( SelectionEvent e ) {
				
				FileDialog fd = new FileDialog( shell, SWT.SAVE );
				fd.setText( "Save Image... " );
				String[] filter = { "*.png;*.PNG;*.jpg;*.JPG;*.jpeg;*.JPEG;*.bmp;*.BMP" };
				String[] filterName = { "Image Files" };
				fd.setFilterExtensions( filter );
				fd.setFilterNames( filterName );

				SimpleDateFormat formatter = new SimpleDateFormat( "yyyy-mm-dd-hhmmss" );
				
				String vmName = SkinUtil.getVmName( config );
				
				fd.setFileName( vmName + "-" + formatter.format( new Date( System.currentTimeMillis() ) ) + ".png" );

				String path = fd.open();
				if ( null == path ) {
					return;
				}

				int i = path.lastIndexOf( '.' );
				String filePath = path;
				String format = "";
				
				if ( -1 != i ) {
					filePath = path.substring( 0, i - 1 );
					format = path.substring( i + 1, path.length() );
					format = format.toLowerCase();
				}

				FileOutputStream fos = null;
				
				try {
					
					fos = new FileOutputStream( filePath + "." + format, true );
					
					ImageLoader loader = new ImageLoader();
					loader.data = new ImageData[] { image.getImageData() };
					
					if ( null == format || format.equals( "png" ) ) {
						loader.save( fos, SWT.IMAGE_PNG );
					} else if ( format.equals( "jpg" ) || format.equals( "jpeg" ) ) {
						loader.save( fos, SWT.IMAGE_JPEG );
					} else if ( format.equals( "bmp" ) ) {
						loader.save( fos, SWT.IMAGE_BMP );
					} else {
						
						MessageBox msg = new MessageBox( shell, SWT.ICON_WARNING );
						msg.setText( "Warning" );
						msg.setMessage( "You have to use specific image formats. (PNG/JPG/BMP)" );
						msg.open();
						
					}
					
				} catch ( IOException ex ) {
					logger.log( Level.SEVERE, ex.getMessage(), ex );
				} finally {
					IOUtil.close( fos );
				}
			}

		} );

		
		MenuItem refreshItem = new MenuItem( menu, SWT.FLAT );
		refreshItem.setText( "Refresh" );
		
		saveItem.addSelectionListener( new SelectionAdapter() {
			
			@Override
			public void widgetSelected( SelectionEvent e ) {
				capture( canvas );
			}

		} );
		
	}

	public void open() {

		shell.open();

		while ( !shell.isDisposed() ) {
			if ( !shell.getDisplay().readAndDispatch() ) {
				shell.getDisplay().sleep();
			}
		}

	}
}