/**
 * 
 *
 * Copyright (C) 2011 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * Hyunjun Son <hj79.son@samsung.com>
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

package org.tizen.emulator.skin;

import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Map.Entry;
import java.util.logging.Level;
import java.util.logging.Logger;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DragDetectEvent;
import org.eclipse.swt.events.DragDetectListener;
import org.eclipse.swt.events.KeyEvent;
import org.eclipse.swt.events.KeyListener;
import org.eclipse.swt.events.MenuDetectEvent;
import org.eclipse.swt.events.MenuDetectListener;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.events.MouseMoveListener;
import org.eclipse.swt.events.MouseTrackAdapter;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.MenuItem;
import org.eclipse.swt.widgets.MessageBox;
import org.eclipse.swt.widgets.Shell;
import org.tizen.emulator.skin.comm.ICommunicator.KeyEventType;
import org.tizen.emulator.skin.comm.ICommunicator.MouseEventType;
import org.tizen.emulator.skin.comm.ICommunicator.SendCommand;
import org.tizen.emulator.skin.comm.sock.SocketCommunicator;
import org.tizen.emulator.skin.comm.sock.data.BooleanData;
import org.tizen.emulator.skin.comm.sock.data.KeyEventData;
import org.tizen.emulator.skin.comm.sock.data.LcdStateData;
import org.tizen.emulator.skin.comm.sock.data.MouseEventData;
import org.tizen.emulator.skin.config.EmulatorConfig;
import org.tizen.emulator.skin.config.EmulatorConfig.ArgsConstants;
import org.tizen.emulator.skin.config.EmulatorConfig.PropertiesConstants;
import org.tizen.emulator.skin.dbi.ColorsType;
import org.tizen.emulator.skin.dbi.RgbType;
import org.tizen.emulator.skin.dbi.RotationType;
import org.tizen.emulator.skin.dialog.AboutDialog;
import org.tizen.emulator.skin.image.ImageRegistry;
import org.tizen.emulator.skin.image.ImageRegistry.IconName;
import org.tizen.emulator.skin.image.ImageRegistry.ImageType;
import org.tizen.emulator.skin.log.SkinLogger;
import org.tizen.emulator.skin.screenshot.ScreenShotDialog;
import org.tizen.emulator.skin.util.SkinRegion;
import org.tizen.emulator.skin.util.SkinRotation;
import org.tizen.emulator.skin.util.SkinRotation.RotationInfo;
import org.tizen.emulator.skin.util.SkinUtil;

/**
 * 
 *
 */
public class EmulatorSkin {

	private Logger logger = SkinLogger.getSkinLogger( EmulatorSkin.class ).getLogger();

	private EmulatorConfig config;
	private Shell shell;
	private ImageRegistry imageRegistry;
	private Canvas lcdCanvas;
	private Image currentImage;
	private Image currentKeyPressedImage;
	private Color hoverColor;
	private boolean isDefaultHoverColor;
	
	private int currentScale;
	private short currentRotationId;
	private int currentAngle;
	private int currentLcdWidth;
	private int currentLcdHeight;
	private SkinRegion currentHoverRegion;

	private int pressedMouseX;
	private int pressedMouseY;
	private boolean isMousePressed;
	private boolean isDragStartedInLCD;
	private boolean isHoverState;
	private boolean isShutdownRequested;
	
	private SocketCommunicator communicator;
	private int windowHandleId;

	protected EmulatorSkin( EmulatorConfig config ) {
		this.config = config;
		this.shell = new Shell( new Display(), SWT.NO_TRIM );
		this.isDefaultHoverColor = true;
	}

	public void setCommunicator( SocketCommunicator communicator ) {
		this.communicator = communicator;
	}

	public int compose() {

		imageRegistry = new ImageRegistry( shell.getDisplay(), config );

		shell.setBackground( shell.getDisplay().getSystemColor( SWT.COLOR_BLACK ) );

		int x = config.getPropertyInt( PropertiesConstants.WINDOW_X, 50 );
		int y = config.getPropertyInt( PropertiesConstants.WINDOW_Y, 50 );
		shell.setLocation( x, y );

		String emulatorName = SkinUtil.makeEmulatorName( config );
		shell.setText( emulatorName );
		
		if( SkinUtil.isWindowsPlatform() ) {
			shell.setImage( imageRegistry.getIcon( IconName.EMULATOR_TITLE_ICO ) );
		}else {
			shell.setImage( imageRegistry.getIcon( IconName.EMULATOR_TITLE ) );
		}
		
		this.lcdCanvas = new Canvas( shell, SWT.EMBEDDED );
		lcdCanvas.setBackground( shell.getDisplay().getSystemColor( SWT.COLOR_BLACK ) );
		
		int lcdWidth = Integer.parseInt( config.getArg( ArgsConstants.RESOLUTION_WIDTH ) );
		int lcdHeight = Integer.parseInt( config.getArg( ArgsConstants.RESOLUTION_HEIGHT ) );
		
		int scale = SkinUtil.getValidScale( config );
		
//		short rotationId = config.getPropertyShort( PropertiesConstants.WINDOW_DIRECTION, (short) 0 );
		// has to be portrait mode at first booting time
		arrangeSkin( lcdWidth, lcdHeight, scale, RotationInfo.PORTRAIT.id() );
		
		decideHoverColor();

		Menu menu = new Menu( shell );
		addMenuItems( menu );
		shell.setMenu( menu );
		
		// sdl uses this handle id.
		windowHandleId = getWindowHandleId( lcdCanvas );
		
		addLCDListener( lcdCanvas );
		addShellListener( shell );

		return windowHandleId;

	}
	
	private int getWindowHandleId( Canvas lcdCanvas ) {
		
		int windowHandleId = 0;
		
		if( SkinUtil.isLinuxPlatform() ) {
			
			try {
				Field field = lcdCanvas.getClass().getField( "embeddedHandle" );
				windowHandleId = field.getInt( lcdCanvas );
				logger.info( "lcdCanvas.embeddedHandle:" + windowHandleId );
			} catch ( IllegalArgumentException e ) {
				logger.log( Level.SEVERE, e.getMessage(), e );
				shutdown();
			} catch ( IllegalAccessException e ) {
				logger.log( Level.SEVERE, e.getMessage(), e );
				shutdown();
			} catch ( SecurityException e ) {
				logger.log( Level.SEVERE, e.getMessage(), e );
				shutdown();
			} catch ( NoSuchFieldException e ) {
				logger.log( Level.SEVERE, e.getMessage(), e );
				shutdown();
			}
			
		}else if( SkinUtil.isWindowsPlatform() ) {
			
			logger.info( "lcdCanvas.handle:" + lcdCanvas.handle );
			windowHandleId = lcdCanvas.handle;
			
		}else if( SkinUtil.isMacPlatform() ) {
			
			//TODO
			
		}else {
			logger.severe( "Not Supported OS platform:" + SWT.getPlatform() );
			System.exit( -1 );
		}

		return windowHandleId;
		
	}
	
	private void decideHoverColor() {
		
		ColorsType colors = config.getDbiContents().getColors();
		if ( null != colors ) {
			RgbType hoverRgb = colors.getHoverColor();
			if ( null != hoverRgb ) {
				Long r = hoverRgb.getR();
				Long g = hoverRgb.getG();
				Long b = hoverRgb.getB();
				if ( null != r && null != g && null != b ) {
					hoverColor = new Color( shell.getDisplay(), new RGB( r.intValue(), g.intValue(), b.intValue() ) );
					isDefaultHoverColor = false;
				}
			}
		}

		if( isDefaultHoverColor ) {
			hoverColor = shell.getDisplay().getSystemColor( SWT.COLOR_WHITE );			
		}

	}
	
	public void open() {

		if ( null == this.communicator ) {
			logger.severe( "communicator is null." );
			shell.close();
			return;
		}

		Display display = this.shell.getDisplay();

		this.shell.open();

		while ( !shell.isDisposed() ) {
			if ( !display.readAndDispatch() ) {
				display.sleep();
			}
		}

		display.dispose();

	}

	private void arrangeSkin( int lcdWidth, int lcdHeight, int scale, short rotationId ) {

		this.currentLcdWidth = lcdWidth;
		this.currentLcdHeight = lcdHeight;
		this.currentScale = scale;
		this.currentRotationId = rotationId;
		this.currentAngle = SkinRotation.getAngle( rotationId );

		if ( null != currentImage ) {
			currentImage.dispose();
		}
		if ( null != currentKeyPressedImage ) {
			currentKeyPressedImage.dispose();
		}

		currentImage = SkinUtil.createScaledImage( imageRegistry, shell, rotationId, scale, ImageType.IMG_TYPE_MAIN );
		currentKeyPressedImage = SkinUtil.createScaledImage( imageRegistry, shell, rotationId, scale,
				ImageType.IMG_TYPE_PRESSED );

		SkinUtil.trimShell( shell, currentImage );
		SkinUtil.adjustLcdGeometry( lcdCanvas, scale, rotationId );

		ImageData imageData = currentImage.getImageData();
		shell.setMinimumSize( imageData.width, imageData.height );
		shell.setSize( imageData.width, imageData.height );

	}

	private void addShellListener( final Shell shell ) {
		
		shell.addListener( SWT.Close, new Listener() {
			@Override
			public void handleEvent( Event event ) {

				if ( isShutdownRequested ) {

					config.setProperty( PropertiesConstants.WINDOW_X, shell.getLocation().x );
					config.setProperty( PropertiesConstants.WINDOW_Y, shell.getLocation().y );
					config.setProperty( PropertiesConstants.WINDOW_SCALE, currentScale );
					config.setProperty( PropertiesConstants.WINDOW_DIRECTION, currentRotationId );

					config.saveProperties();

					if ( null != currentImage ) {
						currentImage.dispose();
					}
					if ( null != currentKeyPressedImage ) {
						currentKeyPressedImage.dispose();
					}

					imageRegistry.dispose();
					
					if( !isDefaultHoverColor ) {
						hoverColor.dispose();
					}

				} else {

					// Skin have to be alive until receiving shutdown request from qemu.
					event.doit = false;
					if ( null != communicator ) {
						communicator.sendToQEMU( SendCommand.CLOSE, null );
					}

				}

			}
		} );

		shell.addPaintListener( new PaintListener() {

			@Override
			public void paintControl( final PaintEvent e ) {

				// general shell does not support native transparency, so draw image with GC.
				if ( null != currentImage ) {
					e.gc.drawImage( currentImage, 0, 0 );
				}

			}
		} );

		shell.addMouseTrackListener( new MouseTrackAdapter() {
			@Override
			public void mouseExit( MouseEvent e ) {
				// MouseMoveListener of shell does not receive event only with MouseMoveListener
				// in case that : hover hardkey -> mouse move into LCD area
				if( isHoverState ) {
					if (currentHoverRegion.width == 0 && currentHoverRegion.height == 0) {
						shell.redraw();
					} else {
						shell.redraw( currentHoverRegion.x, currentHoverRegion.y,
								currentHoverRegion.width + 1, currentHoverRegion.height + 1, false );
					}
					isHoverState = false;
					currentHoverRegion.width = currentHoverRegion.height = 0;
				}
			}
			
		} );
		
		shell.addMouseMoveListener( new MouseMoveListener() {

			@Override
			public void mouseMove( MouseEvent e ) {
				if ( EmulatorSkin.this.isMousePressed ) {
					if ( 0 == e.button ) { // left button

						SkinRegion hardkeyRegion = SkinUtil.getHardKeyArea( e.x, e.y, currentRotationId, currentScale );

						if ( null == hardkeyRegion ) {
							Point previouseLocation = shell.getLocation();
							int x = previouseLocation.x + ( e.x - EmulatorSkin.this.pressedMouseX );
							int y = previouseLocation.y + ( e.y - EmulatorSkin.this.pressedMouseY );

							shell.setLocation( x, y );
						}

					}
				} else {

					SkinRegion region = SkinUtil.getHardKeyArea( e.x, e.y, currentRotationId, currentScale );

					if ( null == region ) {
						if( isHoverState ) {
							if (currentHoverRegion.width == 0 && currentHoverRegion.height == 0) {
								shell.redraw();
							} else {
								shell.redraw( currentHoverRegion.x, currentHoverRegion.y,
										currentHoverRegion.width + 1, currentHoverRegion.height + 1, false );
							}
							isHoverState = false;
							currentHoverRegion.width = currentHoverRegion.height = 0;
						}
					} else {
						isHoverState = true;
						GC gc = new GC( shell );
						gc.setLineWidth( 1 );
						gc.setForeground( hoverColor );
						gc.drawRectangle( region.x, region.y, region.width, region.height );
						currentHoverRegion = region;
						gc.dispose();
					}

				}

			}
		} );

		shell.addMouseListener( new MouseListener() {

			@Override
			public void mouseUp( MouseEvent e ) {
				if ( 1 == e.button ) { // left button
					logger.info( "mouseUp in Skin" );
					EmulatorSkin.this.pressedMouseX = 0;
					EmulatorSkin.this.pressedMouseY = 0;
					EmulatorSkin.this.isMousePressed = false;

					int keyCode = SkinUtil.getHardKeyCode( e.x, e.y, currentRotationId, currentScale );

					if ( EmulatorConstants.UNKNOWN_KEYCODE != keyCode ) {
						if (currentHoverRegion.width == 0 && currentHoverRegion.height == 0) {
							shell.redraw();
						} else {
							shell.redraw( currentHoverRegion.x, currentHoverRegion.y,
									currentHoverRegion.width + 1, currentHoverRegion.height + 1, false );
						}
						KeyEventData keyEventData = new KeyEventData( KeyEventType.RELEASED.value(), keyCode );
						communicator.sendToQEMU( SendCommand.SEND_HARD_KEY_EVENT, keyEventData );
					}
					
				}
			}

			@Override
			public void mouseDown( MouseEvent e ) {
				if ( 1 == e.button ) { // left button
					logger.info( "mouseDown in Skin" );
					EmulatorSkin.this.pressedMouseX = e.x;
					EmulatorSkin.this.pressedMouseY = e.y;

					EmulatorSkin.this.isMousePressed = true;

					int keyCode = SkinUtil.getHardKeyCode( e.x, e.y, currentRotationId, currentScale );

					if ( EmulatorConstants.UNKNOWN_KEYCODE != keyCode ) {
						// draw the button region as the cropped keyPressed image area
						SkinRegion region = SkinUtil.getHardKeyArea( e.x, e.y, currentRotationId, currentScale );

						if ( null != currentKeyPressedImage ) {
							GC gc = new GC( shell );
							gc.drawImage( currentKeyPressedImage,
									region.x + 1, region.y + 1, region.width - 1, region.height - 1, //src
									region.x + 1, region.y + 1, region.width - 1, region.height - 1 ); //dst
							gc.dispose();
						}

						KeyEventData keyEventData = new KeyEventData( KeyEventType.PRESSED.value(), keyCode );
						communicator.sendToQEMU( SendCommand.SEND_HARD_KEY_EVENT, keyEventData );
					}
				}
			}

			@Override
			public void mouseDoubleClick( MouseEvent e ) {
			}
		} );

	}

	private void addLCDListener( final Canvas canvas ) {

		// remove 'input method' menu item 
		canvas.addMenuDetectListener( new MenuDetectListener() {
			@Override
			public void menuDetected( MenuDetectEvent e ) {
				Menu menu = shell.getMenu();
				lcdCanvas.setMenu( menu );
				menu.setVisible( true );
				e.doit = false;
			}
		} );

		canvas.addDragDetectListener( new DragDetectListener() {

			@Override
			public void dragDetected( DragDetectEvent e ) {
				if( logger.isLoggable( Level.FINE ) ) {
					logger.fine( "dragDetected e.button:" + e.button );
				}
				if ( 1 == e.button && // left button
						e.x > 0 && e.x < canvas.getSize().x && e.y > 0 && e.y < canvas.getSize().y ) {

					if( logger.isLoggable( Level.FINE ) ) {
						logger.fine( "dragDetected in LCD" );
					}
					EmulatorSkin.this.isDragStartedInLCD = true;

				}
			}
		} );

		canvas.addMouseMoveListener( new MouseMoveListener() {

			@Override
			public void mouseMove( MouseEvent e ) {
				if ( true == EmulatorSkin.this.isDragStartedInLCD ) {
					int eventType = MouseEventType.DRAG.value();
					Point canvasSize = canvas.getSize();

					if ( e.x <= 0 ) {
						e.x = 1;
						eventType = MouseEventType.UP.value();
						EmulatorSkin.this.isDragStartedInLCD = false;
					} else if ( e.x >= canvasSize.x ) {
						e.x = canvasSize.x - 1;
						eventType = MouseEventType.UP.value();
						EmulatorSkin.this.isDragStartedInLCD = false;
					}

					if ( e.y <= 0 ) {
						e.y = 1;
						eventType = MouseEventType.UP.value();
						EmulatorSkin.this.isDragStartedInLCD = false;
					} else if ( e.y >= canvasSize.y ) {
						e.y = canvasSize.y - 1;
						eventType = MouseEventType.UP.value();
						EmulatorSkin.this.isDragStartedInLCD = false;
					}

					int[] geometry = SkinUtil.convertMouseGeometry( e.x, e.y, currentLcdWidth, currentLcdHeight,
							currentScale, currentAngle );
					
					MouseEventData mouseEventData = new MouseEventData( eventType, geometry[0], geometry[1], 0 );
					communicator.sendToQEMU( SendCommand.SEND_MOUSE_EVENT, mouseEventData );
				}
			}
		} );

		canvas.addMouseListener( new MouseListener() {

			@Override
			public void mouseUp( MouseEvent e ) {
				if ( 1 == e.button ) { // left button

					int[] geometry = SkinUtil.convertMouseGeometry( e.x, e.y, currentLcdWidth, currentLcdHeight,
							currentScale, currentAngle );

					logger.info( "mouseUp in LCD" + " x:" + geometry[0] + " y:" + geometry[1] );
					MouseEventData mouseEventData = new MouseEventData( MouseEventType.UP.value(), geometry[0],
							geometry[1], 0 );
					communicator.sendToQEMU( SendCommand.SEND_MOUSE_EVENT, mouseEventData );
					if ( true == EmulatorSkin.this.isDragStartedInLCD ) {
						EmulatorSkin.this.isDragStartedInLCD = false;
					}
				}
			}

			@Override
			public void mouseDown( MouseEvent e ) {
				if ( 1 == e.button ) { // left button

					int[] geometry = SkinUtil.convertMouseGeometry( e.x, e.y, currentLcdWidth, currentLcdHeight,
							currentScale, currentAngle );

					logger.info( "mouseDown in LCD" + " x:" + geometry[0] + " y:" + geometry[1] );
					MouseEventData mouseEventData = new MouseEventData( MouseEventType.DOWN.value(), geometry[0],
							geometry[1], 0 );
					communicator.sendToQEMU( SendCommand.SEND_MOUSE_EVENT, mouseEventData );
				}
			}

			@Override
			public void mouseDoubleClick( MouseEvent e ) {
			}
		} );

		canvas.addKeyListener( new KeyListener() {

			@Override
			public void keyReleased( KeyEvent e ) {
				logger.info( "key released. key event:" + e );
				int keyCode = e.keyCode | e.stateMask;

				KeyEventData keyEventData = new KeyEventData( KeyEventType.RELEASED.value(), keyCode );
				communicator.sendToQEMU( SendCommand.SEND_KEY_EVENT, keyEventData );
			}

			@Override
			public void keyPressed( KeyEvent e ) {
				logger.info( "key pressed. key event:" + e );
				int keyCode = e.keyCode | e.stateMask;
				KeyEventData keyEventData = new KeyEventData( KeyEventType.PRESSED.value(), keyCode );
				communicator.sendToQEMU( SendCommand.SEND_KEY_EVENT, keyEventData );
			}

		} );

	}

	private void addMenuItems( final Menu menu ) {

		final MenuItem deviceInfoItem = new MenuItem( menu, SWT.PUSH );

		String emulatorName = SkinUtil.makeEmulatorName( config );
		deviceInfoItem.setText( emulatorName );
		deviceInfoItem.setImage( imageRegistry.getIcon( IconName.DEVICE_INFO ) );
		//FIXME
//		deviceInfoItem.setEnabled( false );
		deviceInfoItem.addSelectionListener( new SelectionAdapter() {
			@Override
			public void widgetSelected( SelectionEvent e ) {
				if( logger.isLoggable( Level.FINE ) ) {
					logger.fine( "Open device info" );
				}
			}
		} );

		deviceInfoItem.addListener( SWT.PaintItem, new Listener() {
			@Override
			public void handleEvent( Event event ) {
				event.gc.drawImage( imageRegistry.getIcon( IconName.DEVICE_INFO ), 0, 0 );
			}
		} );
		
		new MenuItem( menu, SWT.SEPARATOR );

		final MenuItem aotItem = new MenuItem( menu, SWT.CHECK );
		aotItem.setText( "Always On Top" );
		//FIXME
		aotItem.setEnabled( false );
		aotItem.addSelectionListener( new SelectionAdapter() {
			private boolean isTop;

			@Override
			public void widgetSelected( SelectionEvent e ) {
				if( logger.isLoggable( Level.FINE ) ) {
					logger.fine( "Select Always On Top. : " + aotItem.getSelection() );
				}
				isTop = !isTop;
				//TODO
			}
		} );

		final MenuItem rotateItem = new MenuItem( menu, SWT.CASCADE );
		rotateItem.setText( "Rotate" );
		rotateItem.setImage( imageRegistry.getIcon( IconName.ROTATE ) );

		Menu rotateMenu = createRotateMenu( menu.getShell() );
		rotateItem.setMenu( rotateMenu );

		final MenuItem scaleItem = new MenuItem( menu, SWT.CASCADE );
		scaleItem.setText( "Scale" );
		scaleItem.setImage( imageRegistry.getIcon( IconName.SCALING ) );
		Menu scaleMenu = createScaleMenu( menu.getShell() );
		scaleItem.setMenu( scaleMenu );

		new MenuItem( menu, SWT.SEPARATOR );

		final MenuItem advancedItem = new MenuItem( menu, SWT.CASCADE );
		advancedItem.setText( "Advanced" );
		advancedItem.setImage( imageRegistry.getIcon( IconName.ADVANCED ) );
		Menu advancedMenu = createAdvancedMenu( menu.getShell() );
		advancedItem.setMenu( advancedMenu );

//		final MenuItem shellItem = new MenuItem( menu, SWT.PUSH );
//		shellItem.setText( "Shell" );
//		shellItem.setImage( imageRegistry.getIcon( IconName.SHELL ) );
//		shellItem.addSelectionListener( new SelectionAdapter() {
//			@Override
//			public void widgetSelected( SelectionEvent e ) {
//				try {
//					//TODO
//					new ProcessBuilder("/usr/bin/gnome-terminal", "--disable-factory", "-x",
//							"ssh", "-p", "1202", "root@localhost").start();
//					/*new ProcessBuilder("/usr/bin/gnome-terminal", "--disable-factory", "-x",
//							"sdb", "shell").start();*/
//				} catch (Exception e2) {
//					//TODO
//				}
//
//				communicator.sendToQEMU( SendCommand.OPEN_SHELL, null );
//			}
//		} );
//
//		new MenuItem( menu, SWT.SEPARATOR );

		MenuItem closeItem = new MenuItem( menu, SWT.PUSH );
		closeItem.setText( "Close" );
		closeItem.setImage( imageRegistry.getIcon( IconName.CLOSE ) );
		closeItem.addSelectionListener( new SelectionAdapter() {
			@Override
			public void widgetSelected( SelectionEvent e ) {
				communicator.sendToQEMU( SendCommand.CLOSE, null );
			}
		} );

	}

	private Menu createRotateMenu( final Shell shell ) {

		Menu menu = new Menu( shell, SWT.DROP_DOWN );

		final List<MenuItem> rotationList = new ArrayList<MenuItem>();

		final short storedDirectionId = config.getPropertyShort( PropertiesConstants.WINDOW_DIRECTION,
				(short) RotationInfo.PORTRAIT.id() );

		Iterator<Entry<Short, RotationType>> iterator = SkinRotation.getRotationIterator();

		while ( iterator.hasNext() ) {

			Entry<Short, RotationType> entry = iterator.next();
			Short rotationId = entry.getKey();
			RotationType section = entry.getValue();

			final MenuItem menuItem = new MenuItem( menu, SWT.RADIO );
			menuItem.setText( section.getName().value() );
			menuItem.setData( rotationId );
			if ( storedDirectionId == rotationId ) {
				menuItem.setSelection( true );
			}

			rotationList.add( menuItem );

		}

		SelectionAdapter selectionAdapter = new SelectionAdapter() {
			@Override
			public void widgetSelected( SelectionEvent e ) {

				MenuItem item = (MenuItem) e.getSource();

				boolean selection = item.getSelection();

				if ( !selection ) {
					return;
				}

				if ( !communicator.isSensorDaemonStarted() ) {

					// reset selection.
					item.setSelection( false );

					for ( MenuItem m : rotationList ) {
						short rotationId = (Short) m.getData();
						if ( rotationId == currentRotationId ) {
							m.setSelection( true );
							break;
						}
					}
					///////////

					MessageBox messageBox = new MessageBox( shell );
					messageBox.setMessage( "Rotation is not ready." );
					messageBox.open();

					return;

				}

				short rotationId = ( (Short) item.getData() );

				arrangeSkin( currentLcdWidth, currentLcdHeight, currentScale, rotationId );
				LcdStateData lcdStateData = new LcdStateData( currentScale, rotationId );
				communicator.sendToQEMU( SendCommand.CHANGE_LCD_STATE, lcdStateData );

			}
		};

		for ( MenuItem menuItem : rotationList ) {
			menuItem.addSelectionListener( selectionAdapter );
		}

		return menu;
	}

	private Menu createScaleMenu( Shell shell ) {

		Menu menu = new Menu( shell, SWT.DROP_DOWN );

		final List<MenuItem> scaleList = new ArrayList<MenuItem>();

		final MenuItem scaleOneItem = new MenuItem( menu, SWT.RADIO );
		scaleOneItem.setText( "1x" );
		scaleOneItem.setData( 100 );
		scaleList.add( scaleOneItem );

		final MenuItem scaleThreeQtrItem = new MenuItem( menu, SWT.RADIO );
		scaleThreeQtrItem.setText( "3/4x" );
		scaleThreeQtrItem.setData( 75 );
		scaleList.add( scaleThreeQtrItem );

		final MenuItem scalehalfItem = new MenuItem( menu, SWT.RADIO );
		scalehalfItem.setText( "1/2x" );
		scalehalfItem.setData( 50 );
		scaleList.add( scalehalfItem );

		final MenuItem scaleOneQtrItem = new MenuItem( menu, SWT.RADIO );
		scaleOneQtrItem.setText( "1/4x" );
		scaleOneQtrItem.setData( 25 );
		scaleList.add( scaleOneQtrItem );

		SelectionAdapter selectionAdapter = new SelectionAdapter() {

			@Override
			public void widgetSelected( SelectionEvent e ) {

				MenuItem item = (MenuItem) e.getSource();

				boolean selection = item.getSelection();

				if ( !selection ) {
					return;
				}

				int scale = (Integer) item.getData();

				arrangeSkin( currentLcdWidth, currentLcdHeight, scale, currentRotationId );
				LcdStateData lcdStateData = new LcdStateData( scale, currentRotationId );
				communicator.sendToQEMU( SendCommand.CHANGE_LCD_STATE, lcdStateData );

			}
		};

		int storedScale = SkinUtil.getValidScale( config );

		for ( MenuItem menuItem : scaleList ) {

			int scale = (Integer) menuItem.getData();
			if ( scale == storedScale ) {
				menuItem.setSelection( true );
			}

			menuItem.addSelectionListener( selectionAdapter );

		}

		return menu;

	}

	private Menu createAdvancedMenu( final Shell shell ) {

		Menu menu = new Menu( shell, SWT.DROP_DOWN );

		final MenuItem screenshotItem = new MenuItem( menu, SWT.PUSH );
		screenshotItem.setText( "Screen Shot" );
		screenshotItem.setImage( imageRegistry.getIcon( IconName.SCREENSHOT ) );
		screenshotItem.addSelectionListener( new SelectionAdapter() {
			private boolean isOpen;
			@Override
			public void widgetSelected( SelectionEvent e ) {
				if( !isOpen ) {
					isOpen = true;
					ScreenShotDialog dialog = new ScreenShotDialog( shell, lcdCanvas, config );
					dialog.open();
					isOpen = false;
				}
			}
		} );

		final MenuItem usbKeyboardItem = new MenuItem( menu, SWT.CASCADE );
		usbKeyboardItem.setText( "USB Keyboard" );
		usbKeyboardItem.setImage( imageRegistry.getIcon( IconName.USB_KEBOARD ) );
		
		Menu usbKeyBoardMenu = new Menu( shell, SWT.DROP_DOWN );

		final MenuItem usbOnItem = new MenuItem( usbKeyBoardMenu, SWT.RADIO );
		usbOnItem.setText( "On" );

		final MenuItem usbOffItem = new MenuItem( usbKeyBoardMenu, SWT.RADIO );
		usbOffItem.setText( "Off" );
		usbOffItem.setSelection( true );
		
		SelectionAdapter usbSelectionAdaptor = new SelectionAdapter() {
			@Override
			public void widgetSelected( SelectionEvent e ) {
				MenuItem item = (MenuItem) e.getSource();
				if ( item.getSelection() ) {
					boolean on = item.equals( usbOnItem );
					communicator
							.sendToQEMU( SendCommand.USB_KBD, new BooleanData( on, SendCommand.USB_KBD.toString() ) );
				}

			}
		};

		usbOnItem.addSelectionListener( usbSelectionAdaptor );
		usbOffItem.addSelectionListener( usbSelectionAdaptor );

		usbKeyboardItem.setMenu( usbKeyBoardMenu );

		final MenuItem aboutItem = new MenuItem( menu, SWT.PUSH );
		aboutItem.setText( "About" );
		aboutItem.setImage( imageRegistry.getIcon( IconName.ABOUT ) );
		aboutItem.addSelectionListener( new SelectionAdapter() {
			private boolean isOpen;
			@Override
			public void widgetSelected( SelectionEvent e ) {
				if( !isOpen ) {
					isOpen = true;
					AboutDialog dialog = new AboutDialog( shell, "Tizen Emulator Info", SWT.DIALOG_TRIM );
					dialog.open();
					isOpen = false;
				}
			}
		} );

		return menu;

	}

	public void shutdown() {

		isShutdownRequested = true;

		if ( !this.shell.isDisposed() ) {
			this.shell.getDisplay().asyncExec( new Runnable() {
				@Override
				public void run() {
					if ( !shell.isDisposed() ) {
						EmulatorSkin.this.shell.close();
					}
				}
			} );
		}

	}

}