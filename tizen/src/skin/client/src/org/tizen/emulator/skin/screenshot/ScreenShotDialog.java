/**
 * Capture a screenshot of the Emulator framebuffer
 *
 * Copyright ( C ) 2011 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * HyunJun Son <hj79.son@samsung.com>
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

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.logging.Level;
import java.util.logging.Logger;

import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.ScrolledComposite;
import org.eclipse.swt.dnd.Clipboard;
import org.eclipse.swt.dnd.ImageTransfer;
import org.eclipse.swt.dnd.Transfer;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.graphics.ImageLoader;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.ToolBar;
import org.eclipse.swt.widgets.ToolItem;
import org.tizen.emulator.skin.EmulatorSkin;
import org.tizen.emulator.skin.comm.ICommunicator.RotationInfo;
import org.tizen.emulator.skin.comm.sock.SocketCommunicator;
import org.tizen.emulator.skin.config.EmulatorConfig;
import org.tizen.emulator.skin.config.EmulatorConfig.ArgsConstants;
import org.tizen.emulator.skin.exception.ScreenShotException;
import org.tizen.emulator.skin.image.ImageRegistry;
import org.tizen.emulator.skin.image.ImageRegistry.IconName;
import org.tizen.emulator.skin.log.SkinLogger;
import org.tizen.emulator.skin.util.IOUtil;
import org.tizen.emulator.skin.util.SkinUtil;
import org.tizen.emulator.skin.util.SwtUtil;
import org.tizen.emulator.skin.util.StringUtil;

public class ScreenShotDialog {

	public final static String DEFAULT_FILE_EXTENSION = "png";
	public final static int CANVAS_MARGIN = 30;
	public final static int TOOLITEM_COOLTIME = 200;

	private Logger logger = SkinLogger.getSkinLogger( ScreenShotDialog.class ).getLogger();

	private Image image;
	private Canvas imageCanvas;
	private Shell shell;
	private ScrolledComposite scrollComposite;
	private Label label;

	private SocketCommunicator communicator;
	private EmulatorSkin emulatorSkin;
	private EmulatorConfig config;

	private RotationInfo currentRotation;
	private boolean reserveImage;
	private ToolItem refreshItem;
	private ToolItem copyItem;
	private ToolItem IncreaseScaleItem;
	private ToolItem DecreaseScaleItem;
	private double scaleLevel;
	/**
	 * @brief constructor
	 * @param Image icon : screenshot window icon resource
	*/
	public ScreenShotDialog( Shell parent, SocketCommunicator communicator, EmulatorSkin emulatorSkin,
			EmulatorConfig config, Image icon ) throws ScreenShotException {

		this.communicator = communicator;
		this.emulatorSkin = emulatorSkin;
		this.config = config;
		scaleLevel = 100d;
		shell = new Shell( Display.getDefault(), SWT.DIALOG_TRIM | SWT.RESIZE | SWT.MAX );
		shell.setText( "Screen Shot - " + SkinUtil.makeEmulatorName( config ) );
		if (icon != null) {
			shell.setImage(icon);
		}

		shell.addListener( SWT.Close, new Listener() {
			@Override
			public void handleEvent( Event event ) {
				if ( null != image ) {
					if ( !reserveImage ) {
						image.dispose();
					}
				}
			}
		} );

		GridLayout gridLayout = new GridLayout();
		gridLayout.marginWidth = 0;
		gridLayout.marginHeight = 0;
		gridLayout.horizontalSpacing = 0;
		gridLayout.verticalSpacing = 0;
		shell.setLayout( gridLayout );

		makeMenuBar( shell );

		scrollComposite = new ScrolledComposite( shell, SWT.V_SCROLL | SWT.H_SCROLL );
		GridData gridData = new GridData( SWT.FILL, SWT.FILL, true, true );
		scrollComposite.setLayoutData( gridData );

		scrollComposite.setExpandHorizontal( true );
		scrollComposite.setExpandVertical( true );

		currentRotation = getCurrentRotation();

		imageCanvas = new Canvas( scrollComposite, SWT.NONE );
		imageCanvas.setBackground( shell.getDisplay().getSystemColor( SWT.COLOR_DARK_GRAY ) );
		imageCanvas.addPaintListener( new PaintListener() {
			@Override
			public void paintControl( PaintEvent e ) {

				logger.fine( "paint image." );

				if ( null != image && !image.isDisposed() ) {
					//e.gc.drawImage( image, CANVAS_MARGIN, CANVAS_MARGIN );
					Rectangle r = image.getBounds();
					logger.info("r.width: " +r.width +", r.height " + r.height);
					e.gc.drawImage(image, 0, 0, r.width, r.height,
							CANVAS_MARGIN, CANVAS_MARGIN, (int)(r.width  * scaleLevel * 1/100), (int)(r.height * scaleLevel * 1/100));
				}

			}
		} );

		scrollComposite.setContent( imageCanvas );

		try {
			clickShutter();
		} catch ( ScreenShotException e ) {
			if ( !shell.isDisposed() ) {
				shell.close();
			}
			throw e;
		}
		
		shell.pack();
		
		label = new Label(shell, SWT.NORMAL );
		label.setText(" Resolution : " + config.getArgInt(ArgsConstants.RESOLUTION_WIDTH) +
				"x" + config.getArgInt(ArgsConstants.RESOLUTION_HEIGHT) + " " + scaleLevel + "%");
		
//		imageCanvas.addMouseMoveListener(new MouseMoveListener() {
//		      public void mouseMove(MouseEvent e) {
//		    		  Rectangle rectangle = imageCanvas.getBounds();
//		    		  if(rectangle.contains(e.x - CANVAS_MARGIN, e.y - CANVAS_MARGIN) == true ) {
//		    			  ImageData imageData = image.getImageData();
//		    			  
//		    			  try {
//		    				  int pixel = imageData.getPixel(e.x - CANVAS_MARGIN , e.y - CANVAS_MARGIN);
//		    				  if(pixel != 0) {
//				    		  PaletteData paletteData = new PaletteData( RED_MASK, GREEN_MASK, BLUE_MASK );
//				    		  RGB rgb =  paletteData.getRGB(pixel);
//				    		  label.setText("R: "+ rgb.red + " G: " + rgb.green + " B: " + rgb.blue);
//				    		  label.update();
//		    			  		}
//		    			  		}catch (IllegalArgumentException e1) {
//		    			  			logger.warning(e1.getMessage());
//		    			  		}
//		    			  
//		    			  } 
//		      		}
//		 });

		Rectangle monitorBound = Display.getDefault().getBounds();
		logger.info("host monitor display bound : " + monitorBound);
		Rectangle emulatorBound = parent.getBounds();
		logger.info("current Emulator window bound : " + emulatorBound);
		Rectangle dialogBound = shell.getBounds();
		logger.info("current ScreenShot Dialog bound : " + dialogBound);

		/* size correction */
		shell.setSize(emulatorBound.width, emulatorBound.height);
		dialogBound = shell.getBounds();
		logger.info("current ScreenShot Dialog bound : " + dialogBound);

		/* location correction */
		int x = emulatorBound.x + emulatorBound.width + 20;
		int y = emulatorBound.y;
		if ((x + dialogBound.width) > (monitorBound.x + monitorBound.width)) {
			x = emulatorBound.x - dialogBound.width - 20;
		}
		shell.setLocation(x, y);
		dialogBound = shell.getBounds();
		logger.info("current ScreenShot Dialog bound : " + dialogBound);

	}

//	private void drawRotatedImage( GC gc, int width, int height ) {
//
//		Transform transform = new Transform( shell.getDisplay() );
//
//		float angle = currentRotation.angle();
//		transform.rotate( angle );
//
//		if ( RotationInfo.LANDSCAPE.equals( currentRotation ) ) {
//			transform.translate( -width - ( 2 * CANVAS_MARGIN ), 0 );
//		} else if ( RotationInfo.REVERSE_PORTRAIT.equals( currentRotation ) ) {
//			transform.translate( -width - ( 2 * CANVAS_MARGIN ), -height - ( 2 * CANVAS_MARGIN ) );
//		} else if ( RotationInfo.REVERSE_LANDSCAPE.equals( currentRotation ) ) {
//			transform.translate( 0, -height - ( 2 * CANVAS_MARGIN ) );
//		}
//		gc.setTransform( transform );
//
//		gc.drawImage( image, CANVAS_MARGIN, CANVAS_MARGIN );
//
//		transform.dispose();
//
//	}

	private void clickShutter() throws ScreenShotException {
		capture();
		arrageImageLayout();
	}

	private void capture() throws ScreenShotException {

		/* DataTranfer dataTranfer = communicator.sendToQEMU( SendCommand.SCREEN_SHOT, null, true );
		byte[] receivedData = communicator.getReceivedData( dataTranfer );

		if ( null != receivedData ) {

			if ( null != this.image ) {
				this.image.dispose();
			}

			int width = config.getArgInt( ArgsConstants.RESOLUTION_WIDTH );
			int height = config.getArgInt( ArgsConstants.RESOLUTION_HEIGHT );
			ImageData imageData = new ImageData( width , height, COLOR_DEPTH, paletteData, 1, receivedData );
			
			RotationInfo rotation = getCurrentRotation();
			imageData = rotateImageData( imageData, rotation );

		 this.image = new Image( Display.getDefault(), imageData );
		 
		 imageCanvas.redraw();
			
		} else {
			throw new ScreenShotException( "Fail to get image data." );
		}*/

		int width = config.getArgInt(ArgsConstants.RESOLUTION_WIDTH);
		int height = config.getArgInt(ArgsConstants.RESOLUTION_HEIGHT);

		int[] array = new int[width * height];
		int result = emulatorSkin.getPixels(array); //from shared memory
		//logger.info("getPixels navtive function returned " + result);

		ImageData imageData = new ImageData(width, height, EmulatorSkin.COLOR_DEPTH, emulatorSkin.getPalette());
		for (int i = 0; i < height; i++) {
			imageData.setPixels(0, i, width, array, i * width);
		}

		RotationInfo rotation = getCurrentRotation();
		imageData = rotateImageData(imageData, rotation);

		if (image != null) {
			image.dispose();
		}
		image = new Image(Display.getDefault(), imageData);
		imageCanvas.redraw();
	}

	private double getScaleLevel() {
		return scaleLevel;
	}

	private void DownScaleLevel() {
		scaleLevel /= 2;
		logger.info("down scaling level : " + scaleLevel);
	}
	
	private void UpScaleLevel() {
		scaleLevel *= 2;
		logger.info("up scaling level : " + scaleLevel);
	}
	
	private void arrageImageLayout() {

		ImageData imageData = image.getImageData();
		scaleLevel = 100d;
		int width = imageData.width + ( 2 * CANVAS_MARGIN );
		int height = imageData.height + ( 2 * CANVAS_MARGIN );
		logger.info("arrageImageLayout width:" + width + ", height: "+ height);
		scrollComposite.setMinSize( width, height );
		
		RotationInfo rotation = getCurrentRotation();
		if ( !currentRotation.equals( rotation ) ) { // reserve changed shell size by user
			shell.pack();
		}

		currentRotation = rotation;

	}
	
	private void scaledImageLayout() {

		ImageData imageData = image.getImageData();

		int width = imageData.width + ( 2 * CANVAS_MARGIN );
		int height = imageData.height + ( 2 * CANVAS_MARGIN );
		logger.info("arrageImageLayout2 width:" + width + ", height: "+ height);
		int reWidth = (int)(width * scaleLevel * 1/100);
		int reHeight = (int)(height * scaleLevel * 1/100);
		logger.info("arrageImageLayout2 Rewidth:" + reWidth + ", Reheight: "+ reHeight);
		scrollComposite.setMinSize( (int)(imageData.width * scaleLevel * 1/100) + ( 2 * CANVAS_MARGIN ), (int)(imageData.height * scaleLevel * 1/100) + ( 2 * CANVAS_MARGIN ));
		
		RotationInfo rotation = getCurrentRotation();
		if ( !currentRotation.equals( rotation ) ) { // reserve changed shell size by user
			shell.pack();
		}

		currentRotation = rotation;

	}
	

	private ImageData rotateImageData( ImageData srcData, RotationInfo rotation ) {

		int direction = SWT.NONE;

		switch ( rotation ) {
		case PORTRAIT:
			return srcData;
		case LANDSCAPE:
			direction = SWT.LEFT;
			break;
		case REVERSE_PORTRAIT:
			direction = SWT.DOWN;
			break;
		case REVERSE_LANDSCAPE:
			direction = SWT.RIGHT;
			break;
		default:
			return srcData;
		}

		ImageData rotatedData = rotateImageData( srcData, direction );
		return rotatedData;

	}

	/*
	 * refrence web page : http://www.java2s.com/Code/Java/SWT-JFace-Eclipse/Rotateandflipanimage.htm
	 */
	private ImageData rotateImageData( ImageData srcData, int direction ) {
		int bytesPerPixel = srcData.bytesPerLine / srcData.width;
		int destBytesPerLine = ( direction == SWT.DOWN ) ? srcData.width * bytesPerPixel : srcData.height
				* bytesPerPixel;
		byte[] newData = new byte[srcData.data.length];
		int width = 0, height = 0;
		for ( int srcY = 0; srcY < srcData.height; srcY++ ) {
			for ( int srcX = 0; srcX < srcData.width; srcX++ ) {
				int destX = 0, destY = 0, destIndex = 0, srcIndex = 0;
				switch ( direction ) {
				case SWT.LEFT: // left 90 degrees
					destX = srcY;
					destY = srcData.width - srcX - 1;
					width = srcData.height;
					height = srcData.width;
					break;
				case SWT.RIGHT: // right 90 degrees
					destX = srcData.height - srcY - 1;
					destY = srcX;
					width = srcData.height;
					height = srcData.width;
					break;
				case SWT.DOWN: // 180 degrees
					destX = srcData.width - srcX - 1;
					destY = srcData.height - srcY - 1;
					width = srcData.width;
					height = srcData.height;
					break;
				}
				destIndex = ( destY * destBytesPerLine ) + ( destX * bytesPerPixel );
				srcIndex = ( srcY * srcData.bytesPerLine ) + ( srcX * bytesPerPixel );
				System.arraycopy( srcData.data, srcIndex, newData, destIndex, bytesPerPixel );
			}
		}
		// destBytesPerLine is used as scanlinePad to ensure that no padding is
		// required
		return new ImageData( width, height, srcData.depth, srcData.palette, destBytesPerLine, newData );

	}

	private RotationInfo getCurrentRotation() {
		short currentRotationId = emulatorSkin.getCurrentRotationId();
		RotationInfo rotationInfo = RotationInfo.getValue( currentRotationId );
		return rotationInfo;
	}

	private void makeMenuBar( final Shell shell ) {

		ToolBar toolBar = new ToolBar( shell, SWT.HORIZONTAL );
		GridData gridData = new GridData( GridData.FILL_HORIZONTAL, GridData.CENTER, true, false );
		toolBar.setLayoutData( gridData );

		ToolItem saveItem = new ToolItem( toolBar, SWT.FLAT );
		saveItem.setImage( ImageRegistry.getInstance().getIcon( IconName.SAVE_SCREEN_SHOT ) );
		saveItem.setToolTipText( "Save to file" );

		saveItem.addSelectionListener( new SelectionAdapter() {
			@Override
			public void widgetSelected( SelectionEvent e ) {

				FileDialog fileDialog = new FileDialog( shell, SWT.SAVE );
				fileDialog.setText( "Save Image" );

				String[] filter = { "*.png;*.PNG;*.jpg;*.JPG;*.jpeg;*.JPEG;*.bmp;*.BMP" };
				fileDialog.setFilterExtensions( filter );

				String[] filterName = { "Image files (*.png *.jpg *.jpeg *.bmp)" };
				fileDialog.setFilterNames( filterName );

				String vmName = SkinUtil.getVmName( config );
				SimpleDateFormat formatter = new SimpleDateFormat( "yyyy-MM-dd-hhmmss" );
				String dateString = formatter.format( new Date( System.currentTimeMillis() ) );

				fileDialog.setFileName( vmName + "-" + dateString + "." + DEFAULT_FILE_EXTENSION );

				String userHome = System.getProperty( "user.home" );
				if ( !StringUtil.isEmpty( userHome ) ) {
					fileDialog.setFilterPath( userHome );
				} else {
					logger.warning( "Cannot find user home path int java System properties." );
				}

				fileDialog.setOverwrite(true);
				String filePath = fileDialog.open();
				saveFile( filePath, fileDialog );

			}

		} );

		copyItem = new ToolItem( toolBar, SWT.FLAT );
		copyItem.setImage( ImageRegistry.getInstance().getIcon( IconName.COPY_SCREEN_SHOT ) );
		copyItem.setToolTipText( "Copy to clipboard" );

		copyItem.addSelectionListener( new SelectionAdapter() {
			@Override
			public void widgetSelected( SelectionEvent e ) {

				if ( null == image || image.isDisposed() ) {
					SkinUtil.openMessage( shell, null, "Fail to copy to clipboard.", SWT.ICON_ERROR, config );
					return;
				}

				copyItem.setEnabled(false);
				shell.getDisplay().asyncExec(new Runnable() {
					public void run() {
						try {
							Thread.sleep(TOOLITEM_COOLTIME);
						} catch (InterruptedException e) {
							e.printStackTrace();
						}
						copyItem.setEnabled(true);
					}
				});

				ImageLoader loader = new ImageLoader();

				ImageData data = null;

				if ( SwtUtil.isWindowsPlatform() ) {
					// change RGB mask
					ImageData imageData = image.getImageData();
					data = new ImageData( imageData.width, imageData.height, imageData.depth, emulatorSkin.getPalette(),
							imageData.bytesPerLine, imageData.data );
				} else {
					data = image.getImageData();
				}

				loader.data = new ImageData[] { data };

				ByteArrayOutputStream bao = new ByteArrayOutputStream();
				loader.save( bao, SWT.IMAGE_PNG );

				ImageData imageData = new ImageData( new ByteArrayInputStream( bao.toByteArray() ) );
				Object[] imageObject = new Object[] { imageData };

				Transfer[] transfer = new Transfer[] { ImageTransfer.getInstance() };

				Clipboard clipboard = new Clipboard( shell.getDisplay() );
				clipboard.setContents( imageObject, transfer );

			}

		} );

		refreshItem = new ToolItem( toolBar, SWT.FLAT );
		refreshItem.setImage( ImageRegistry.getInstance().getIcon( IconName.REFRESH_SCREEN_SHOT ) );
		refreshItem.setToolTipText( "Refresh image" );

		refreshItem.addSelectionListener( new SelectionAdapter() {
			@Override
			public void widgetSelected( SelectionEvent e ) {

				refreshItem.setEnabled(false);
				shell.getDisplay().asyncExec(new Runnable() {
					public void run() {
						try {
							Thread.sleep(TOOLITEM_COOLTIME);
						} catch (InterruptedException e) {
							e.printStackTrace();
						}
						refreshItem.setEnabled(true);
					}
				});

				Point dialogSize = shell.getSize();

				try {
					clickShutter();
				} catch ( ScreenShotException ex ) {
					logger.log( Level.SEVERE, "Fail to create a screen shot.", ex );
					SkinUtil.openMessage( shell, null, "Fail to create a screen shot.", SWT.ERROR, config );
				}

				/* restoration */
				if (shell.getSize() != dialogSize) {
					shell.setSize(dialogSize);
				}
			}

		} );

		IncreaseScaleItem = new ToolItem( toolBar, SWT.FLAT );
		IncreaseScaleItem.setImage( ImageRegistry.getInstance().getIcon( IconName.INCREASE_SCALE ) );
		IncreaseScaleItem.setToolTipText( "Increase view size" );
		IncreaseScaleItem.addSelectionListener( new SelectionAdapter() {
			@Override
			public void widgetSelected( SelectionEvent e ) {
				double level = getScaleLevel();
				Point dialogSize = shell.getSize();	
				
				UpScaleLevel();
				imageCanvas.redraw();
				scaledImageLayout();
				label.setText(" Resolution : " + config.getArgInt(ArgsConstants.RESOLUTION_WIDTH) +
						"x" + config.getArgInt(ArgsConstants.RESOLUTION_HEIGHT) + " " + scaleLevel + "%");
				label.update();
				
				if(level >= 400d) {
					IncreaseScaleItem.setEnabled(false);
					DecreaseScaleItem.setEnabled(true);
				}else {
					IncreaseScaleItem.setEnabled(true);
					DecreaseScaleItem.setEnabled(true);	
				}
				
				/* restoration */
				if (shell.getSize() != dialogSize) {
					shell.setSize(dialogSize);
				}
			}

		} );

		DecreaseScaleItem = new ToolItem( toolBar, SWT.FLAT );
		DecreaseScaleItem.setImage( ImageRegistry.getInstance().getIcon( IconName.DECREASE_SCALE ) );
		DecreaseScaleItem.setToolTipText( "Decrease view size" );
		DecreaseScaleItem.addSelectionListener( new SelectionAdapter() {
			@Override
			public void widgetSelected( SelectionEvent e ) {
				double level = getScaleLevel();
				Point dialogSize = shell.getSize();
				
				DownScaleLevel();
				imageCanvas.redraw();
				scaledImageLayout();
				label.setText(" Resolution : " + config.getArgInt(ArgsConstants.RESOLUTION_WIDTH) +
						"x" + config.getArgInt(ArgsConstants.RESOLUTION_HEIGHT) + " " + scaleLevel + "%");
				label.update();
			
				if(level <= 25) {
					DecreaseScaleItem.setEnabled(false);
					IncreaseScaleItem.setEnabled(true);
				}else {
					DecreaseScaleItem.setEnabled(true);
					IncreaseScaleItem.setEnabled(true);
				}
					
				/* restoration */
				if (shell.getSize() != dialogSize) {
					shell.setSize(dialogSize);
				}
			}

		} );

		
	}

	private void saveFile( String fileFullPath, FileDialog fileDialog ) {

		if ( null == fileFullPath ) {
			return;
		}

		String format = "";
		String[] split = fileFullPath.split( "\\." );

		if ( 1 < split.length ) {

			format = split[split.length - 1];

			if ( new File( split[split.length - 2] ).isDirectory() ) {
				// There is no file name.
				SkinUtil.openMessage( shell, null, "Use correct file name.", SWT.ICON_WARNING, config );
				String path = fileDialog.open();
				saveFile( path, fileDialog );

			}

		}

		FileOutputStream fos = null;

		try {

			if ( StringUtil.isEmpty( format ) ) {
				if ( fileFullPath.endsWith( "." ) ) {
					fileFullPath += DEFAULT_FILE_EXTENSION;
				} else {
					fileFullPath += "." + DEFAULT_FILE_EXTENSION;
				}
			}

			ImageLoader loader = new ImageLoader();
			loader.data = new ImageData[] { image.getImageData() };

			if ( StringUtil.isEmpty( format ) || format.equalsIgnoreCase( "png" ) ) {
				fos = new FileOutputStream( fileFullPath, false );
				loader.save( fos, SWT.IMAGE_PNG );
			} else if ( format.equalsIgnoreCase( "jpg" ) || format.equalsIgnoreCase( "jpeg" ) ) {
				fos = new FileOutputStream( fileFullPath, false );
				loader.save( fos, SWT.IMAGE_JPEG );
			} else if ( format.equalsIgnoreCase( "bmp" ) ) {
				fos = new FileOutputStream( fileFullPath, false );
				loader.save( fos, SWT.IMAGE_BMP );
			} else {

				SkinUtil.openMessage( shell, null, "Use the specified image formats. ( PNG / JPG / JPEG / BMP )",
						SWT.ICON_WARNING, config );
				String path = fileDialog.open();
				saveFile( path, fileDialog );

			}

		} catch ( FileNotFoundException ex ) {

			logger.log( Level.WARNING, "Use correct file name.", ex );
			SkinUtil.openMessage( shell, null, "Use correct file name.", SWT.ICON_WARNING, config );
			String path = fileDialog.open();
			saveFile( path, fileDialog );

		} catch ( Exception ex ) {

			logger.log( Level.SEVERE, "Fail to save this image file.", ex );
			SkinUtil.openMessage( shell, null, "Fail to save this image file.", SWT.ERROR, config );
			String path = fileDialog.open();
			saveFile( path, fileDialog );

		} finally {
			IOUtil.close( fos );
		}

	}

	public void open() {

		if ( shell.isDisposed() ) {
			return;
		}

		shell.open();

		while ( !shell.isDisposed() ) {
			if ( !shell.getDisplay().readAndDispatch() ) {
				if ( reserveImage ) {
					break;
				} else {
					shell.getDisplay().sleep();
				}
			}
		}

	}

	public void setEmulatorSkin( EmulatorSkin emulatorSkin ) {
		this.emulatorSkin = emulatorSkin;
	}

	public void setReserveImage( boolean reserveImage ) {
		this.reserveImage = reserveImage;
	}

	public Shell getShell() {
		return shell;
	}

}
