/**
 * Emulator Skin Process
 *
 * Copyright (C) 2011 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * HyunJun Son <hj79.son@samsung.com>
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

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.Map.Entry;
import java.util.logging.Level;
import java.util.logging.Logger;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.FocusEvent;
import org.eclipse.swt.events.FocusListener;
import org.eclipse.swt.events.KeyEvent;
import org.eclipse.swt.events.KeyListener;
import org.eclipse.swt.events.MenuDetectEvent;
import org.eclipse.swt.events.MenuDetectListener;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.events.MouseMoveListener;
import org.eclipse.swt.events.MouseWheelListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.MenuItem;
import org.eclipse.swt.widgets.Shell;
import org.tizen.emulator.skin.comm.ICommunicator.KeyEventType;
import org.tizen.emulator.skin.comm.ICommunicator.MouseButtonType;
import org.tizen.emulator.skin.comm.ICommunicator.MouseEventType;
import org.tizen.emulator.skin.comm.ICommunicator.Scale;
import org.tizen.emulator.skin.comm.ICommunicator.SendCommand;
import org.tizen.emulator.skin.comm.sock.SocketCommunicator;
import org.tizen.emulator.skin.comm.sock.data.BooleanData;
import org.tizen.emulator.skin.comm.sock.data.KeyEventData;
import org.tizen.emulator.skin.comm.sock.data.LcdStateData;
import org.tizen.emulator.skin.comm.sock.data.MouseEventData;
import org.tizen.emulator.skin.config.EmulatorConfig;
import org.tizen.emulator.skin.config.EmulatorConfig.ArgsConstants;
import org.tizen.emulator.skin.config.EmulatorConfig.SkinPropertiesConstants;
import org.tizen.emulator.skin.dbi.ColorsType;
import org.tizen.emulator.skin.dbi.KeyMapType;
import org.tizen.emulator.skin.dbi.RgbType;
import org.tizen.emulator.skin.dbi.RotationType;
import org.tizen.emulator.skin.dialog.AboutDialog;
import org.tizen.emulator.skin.dialog.DetailInfoDialog;
import org.tizen.emulator.skin.dialog.RamdumpDialog;
import org.tizen.emulator.skin.image.ImageRegistry;
import org.tizen.emulator.skin.image.ImageRegistry.IconName;
import org.tizen.emulator.skin.info.SkinInformation;
import org.tizen.emulator.skin.layout.GeneralPurposeSkinComposer;
import org.tizen.emulator.skin.layout.ISkinComposer;
import org.tizen.emulator.skin.layout.PhoneShapeSkinComposer;
import org.tizen.emulator.skin.log.SkinLogger;
import org.tizen.emulator.skin.screenshot.ScreenShotDialog;
import org.tizen.emulator.skin.util.SkinRotation;
import org.tizen.emulator.skin.util.SkinUtil;
import org.tizen.emulator.skin.util.SwtUtil;
import org.tizen.emulator.skin.window.ControlPanel;

/**
 *
 *
 */
public class EmulatorSkin {

	public class SkinReopenPolicy {

		private EmulatorSkin reopenSkin;
		private boolean reopen;

		private SkinReopenPolicy( EmulatorSkin reopenSkin, boolean reopen ) {
			this.reopenSkin = reopenSkin;
			this.reopen = reopen;
		}

		public EmulatorSkin getReopenSkin() {
			return reopenSkin;
		}

		public boolean isReopen() {
			return reopen;
		}

	}

	private static Logger logger =
			SkinLogger.getSkinLogger(EmulatorSkin.class).getLogger();

	protected EmulatorConfig config;
	protected EmulatorFingers finger;
	protected Shell shell;
	protected ImageRegistry imageRegistry;
	protected Canvas lcdCanvas;
	protected SkinInformation skinInfo;
	protected ISkinComposer skinComposer;

	protected EmulatorSkinState currentState;

	private boolean isDragStartedInLCD;
	private boolean isShutdownRequested;
	private boolean isAboutToReopen;
	private boolean isOnTop;
	private boolean isControlPanel;
	private boolean isOnKbd;

	private Menu contextMenu;
	private MenuItem panelItem; /* key window menu */
	public ControlPanel controlPanel;
	public Color colorPairTag;
	public Canvas pairTagCanvas;
	protected ScreenShotDialog screenShotDialog;

	protected SocketCommunicator communicator;
	private Listener shellCloseListener;
	private MenuDetectListener shellMenuDetectListener;

	//private DragDetectListener canvasDragDetectListener;
	private MouseMoveListener canvasMouseMoveListener;
	private MouseListener canvasMouseListener;
	private MouseWheelListener canvasMouseWheelListener;
	private KeyListener canvasKeyListener;
	private MenuDetectListener canvasMenuDetectListener;
	private FocusListener canvasFocusListener;

	private LinkedList<KeyEventData> pressedKeyEventList;

	private EmulatorSkin reopenSkin;
	
	/**
	 * @brief constructor
	 * @param config : configuration of emulator skin
	 * @param isOnTop : always on top flag
	*/
	protected EmulatorSkin(EmulatorSkinState state, EmulatorFingers finger, EmulatorConfig config, SkinInformation skinInfo, boolean isOnTop) {
		this.finger = finger;
		this.config = config;
		this.skinInfo = skinInfo;
		this.isOnTop = isOnTop;
		this.isControlPanel = false;
		this.pressedKeyEventList = new LinkedList<KeyEventData>();

		int style = SWT.NO_TRIM | SWT.DOUBLE_BUFFERED;
//		if (skinInfo.isPhoneShape() == false) {
//			style = SWT.TITLE | SWT.CLOSE | SWT.MIN | SWT.BORDER;
//		}
		this.shell = new Shell(Display.getDefault(), style);
		if (isOnTop == true) {
			SkinUtil.setTopMost(shell, true);
		}

		/* generate a pair tag color of key window */
		int red = (int) (Math.random() * 256);
		int green = (int) (Math.random() * 256);
		int blue = (int) (Math.random() * 256);
		this.colorPairTag = new Color(shell.getDisplay(), new RGB(red, green, blue));

		this.currentState = state;
	}

	public void setCommunicator(SocketCommunicator communicator) {
		this.communicator = communicator;
		this.finger.setCommunicator(this.communicator);
	}

	public long initLayout() {
		imageRegistry = ImageRegistry.getInstance();

		if (skinInfo.isPhoneShape() == true) { /* phone shape skin */
			skinComposer = new PhoneShapeSkinComposer(config, shell,
					currentState, imageRegistry, communicator);

			((PhoneShapeSkinComposer) skinComposer).addPhoneShapeListener(shell);
		} else { /* general purpose skin */
			skinComposer = new GeneralPurposeSkinComposer(config, this,
					shell, currentState, imageRegistry);

			((GeneralPurposeSkinComposer) skinComposer).addGeneralPurposeListener(shell);
		}

		lcdCanvas = skinComposer.compose();

		/* load a hover color */
		currentState.setHoverColor(loadHoverColor());

		/* added event handlers */
		addShellListener(shell);
		addCanvasListener(shell, lcdCanvas);

		/* attach a menu */
		this.isOnKbd = false;
		setMenu();

		return 0;
	}

	private void setMenu() {
		contextMenu = new Menu(shell);

		addMenuItems(shell, contextMenu);

		shell.setMenu(contextMenu);
	}

//	private void readyToReopen( EmulatorSkin sourceSkin, boolean isOnTop ) {
//
//		logger.info( "Start Changing AlwaysOnTop status" );
//
//		sourceSkin.reopenSkin = new EmulatorSkin( sourceSkin.config, isOnTop );
//
//		sourceSkin.reopenSkin.lcdCanvas = sourceSkin.lcdCanvas;
//		Point previousLocation = sourceSkin.shell.getLocation();
//
//		sourceSkin.reopenSkin.composeInternal( sourceSkin.lcdCanvas, previousLocation.x, previousLocation.y,
//				sourceSkin.currentLcdWidth, sourceSkin.currentLcdHeight, sourceSkin.currentScale,
//				sourceSkin.currentRotationId, sourceSkin.isOnKbd );
//
//		sourceSkin.reopenSkin.windowHandleId = sourceSkin.windowHandleId;
//
//		sourceSkin.reopenSkin.communicator = sourceSkin.communicator;
//		sourceSkin.reopenSkin.communicator.resetSkin( reopenSkin );
//
//		sourceSkin.isAboutToReopen = true;
//		sourceSkin.isShutdownRequested = true;
//
//		if ( sourceSkin.isScreenShotOpened && ( null != sourceSkin.screenShotDialog ) ) {
//			sourceSkin.screenShotDialog.setReserveImage( true );
//			sourceSkin.screenShotDialog.setEmulatorSkin( reopenSkin );
//			reopenSkin.isScreenShotOpened = true;
//			reopenSkin.screenShotDialog = sourceSkin.screenShotDialog;
//			// see open() method to know next logic for screenshot dialog.
//		}
//
//		sourceSkin.lcdCanvas.setParent( reopenSkin.shell );
//		sourceSkin.shell.close();
//
//	}

	private Color loadHoverColor() {
		ColorsType colors = config.getDbiContents().getColors();

		if (null != colors) {
			RgbType hoverRgb = colors.getHoverColor();
			if (null != hoverRgb) {
				Long r = hoverRgb.getR();
				Long g = hoverRgb.getG();
				Long b = hoverRgb.getB();
				if (null != r && null != g && null != b) {
					Color hoverColor = new Color(shell.getDisplay(),
							new RGB(r.intValue(), g.intValue(), b.intValue()));
					return hoverColor;
				}
			}
		}

		/* white */
		return (new Color(shell.getDisplay(), new RGB(255, 255, 255)));
	}

	public SkinReopenPolicy open() {

		if ( null == this.communicator ) {
			logger.severe( "communicator is null." );
			return null;
		}

		Display display = this.shell.getDisplay();

		this.shell.open();

		// logic only for reopen case ///////
//		if ( isScreenShotOpened && ( null != screenShotDialog ) ) {
//			try {
//				screenShotDialog.setReserveImage( false );
//				screenShotDialog.open();
//			} finally {
//				isScreenShotOpened = false;
//			}
//		}
		// ///////////////////////////////////

		while ( !shell.isDisposed() ) {
			if ( !display.readAndDispatch() ) {
				display.sleep();
			}
		}

		return new SkinReopenPolicy(reopenSkin, isAboutToReopen);

	}

	protected void skinFinalize() {
		skinComposer.composerFinalize();
	}

	private void addShellListener(final Shell shell) {

		shellCloseListener = new Listener() {
			@Override
			public void handleEvent(Event event) {

				if (isShutdownRequested) {

					removeShellListeners();
					removeCanvasListeners();

					if (!isAboutToReopen) {
						/* close the screen shot window */
						if (null != screenShotDialog) {
							Shell scShell = screenShotDialog.getShell();
							if (!scShell.isDisposed()) {
								scShell.close();
							}
							screenShotDialog = null;
						}

						/* close the control panel window */
						if (null != controlPanel) {
							Shell cpShell = controlPanel.getShell();
							if (!cpShell.isDisposed()) {
								cpShell.close();
							}
							controlPanel = null;

							if (colorPairTag != null) {
								colorPairTag.dispose();
							}
						}

						/* save config only for emulator close */
						config.setSkinProperty(
								SkinPropertiesConstants.WINDOW_X, shell.getLocation().x);
						config.setSkinProperty(
								SkinPropertiesConstants.WINDOW_Y, shell.getLocation().y);
						config.setSkinProperty(
								SkinPropertiesConstants.WINDOW_SCALE, currentState.getCurrentScale());
						config.setSkinProperty(
								SkinPropertiesConstants.WINDOW_ROTATION, currentState.getCurrentRotationId());
						config.setSkinProperty(
								SkinPropertiesConstants.WINDOW_ONTOP, Boolean.toString(isOnTop));
						config.saveSkinProperties();
					}

					if (currentState.getCurrentImage() != null) {
						currentState.getCurrentImage().dispose();
					}
					if (currentState.getCurrentKeyPressedImage() != null) {
						currentState.getCurrentKeyPressedImage().dispose();
					}

					if (currentState.getHoverColor() != null) {
						currentState.getHoverColor().dispose();
					}

					skinFinalize();

				} else {

					// Skin have to be alive until receiving shutdown request from qemu.
					event.doit = false;
					if ( null != communicator ) {
						communicator.sendToQEMU( SendCommand.CLOSE, null );
					}

				}

			}
		};

		shell.addListener( SWT.Close, shellCloseListener );

		shellMenuDetectListener = new MenuDetectListener() {
			@Override
			public void menuDetected(MenuDetectEvent e) {
				Menu menu = EmulatorSkin.this.contextMenu;

				if (menu != null) {
					shell.setMenu(menu);
					menu.setVisible(true);
					e.doit = false;
				} else {
					shell.setMenu(null);
				}
			}
		};

		shell.addMenuDetectListener(shellMenuDetectListener);
	}

	private void removeShellListeners() {
		if (null != shellCloseListener) {
			shell.removeListener(SWT.Close, shellCloseListener);
		}

		if (null != shellMenuDetectListener) {
			shell.removeMenuDetectListener(shellMenuDetectListener);
		}
	}

	private void addCanvasListener(final Shell shell, final Canvas canvas) {
		/* menu */
		canvasMenuDetectListener = new MenuDetectListener() {
			@Override
			public void menuDetected(MenuDetectEvent e) {
				Menu menu = EmulatorSkin.this.contextMenu;

				if (menu != null && EmulatorSkin.this.isDragStartedInLCD == false) {
					lcdCanvas.setMenu(menu);
					menu.setVisible(true);
					e.doit = false;
				} else {
					lcdCanvas.setMenu(null);
				}
			}
		};

		// remove 'input method' menu item ( avoid bug )
		canvas.addMenuDetectListener(canvasMenuDetectListener);

		/* focus */
		canvasFocusListener = new FocusListener() {
			@Override
			public void focusGained(FocusEvent event) {
				logger.info("gain focus");
			}

			public void focusLost(FocusEvent event) {
				logger.info("lost focus");

				/* key event compensation */
				if (pressedKeyEventList.isEmpty() == false) {
					for (KeyEventData data : pressedKeyEventList) {
						KeyEventData keyEventData = new KeyEventData(
								KeyEventType.RELEASED.value(), data.keycode,
								data.stateMask, data.keyLocation);
						communicator.sendToQEMU(SendCommand.SEND_KEY_EVENT, keyEventData);

						logger.info("auto release : keycode=" + keyEventData.keycode +
								", stateMask=" + keyEventData.stateMask +
								", keyLocation=" + keyEventData.keyLocation);
					}
				}

				pressedKeyEventList.clear();
			}
		};

		lcdCanvas.addFocusListener(canvasFocusListener);

		/* mouse event */
		/*canvasDragDetectListener = new DragDetectListener() {

			@Override
			public void dragDetected( DragDetectEvent e ) {
				if ( logger.isLoggable( Level.FINE ) ) {
					logger.fine( "dragDetected e.button:" + e.button );
				}
				if ( 1 == e.button && // left button
						e.x > 0 && e.x < canvas.getSize().x && e.y > 0 && e.y < canvas.getSize().y ) {

					if ( logger.isLoggable( Level.FINE ) ) {
						logger.fine( "dragDetected in LCD" );
					}
					EmulatorSkin.this.isDragStartedInLCD = true;

				}
			}
		};

		canvas.addDragDetectListener( canvasDragDetectListener );*/

		canvasMouseMoveListener = new MouseMoveListener() {

			@Override
			public void mouseMove( MouseEvent e ) {
				if ( true == EmulatorSkin.this.isDragStartedInLCD ) { //true = mouse down
					int eventType = MouseEventType.DRAG.value();
					Point canvasSize = canvas.getSize();

					if ( e.x < 0 ) {
						e.x = 0;
						eventType = MouseEventType.RELEASE.value();
						EmulatorSkin.this.isDragStartedInLCD = false;
					} else if ( e.x >= canvasSize.x ) {
						e.x = canvasSize.x - 1;
						eventType = MouseEventType.RELEASE.value();
						EmulatorSkin.this.isDragStartedInLCD = false;
					}

					if ( e.y < 0 ) {
						e.y = 0;
						eventType = MouseEventType.RELEASE.value();
						EmulatorSkin.this.isDragStartedInLCD = false;
					} else if ( e.y >= canvasSize.y ) {
						e.y = canvasSize.y - 1;
						eventType = MouseEventType.RELEASE.value();
						EmulatorSkin.this.isDragStartedInLCD = false;
					}

					int[] geometry = SkinUtil.convertMouseGeometry(e.x, e.y,
							currentState.getCurrentResolutionWidth(),
							currentState.getCurrentResolutionHeight(),
							currentState.getCurrentScale(), currentState.getCurrentAngle());

					if(SwtUtil.isMacPlatform()) {
						//eventType = MouseEventType.DRAG.value();
						if(finger.getMultiTouchEnable() == 1) {
							logger.info("maruFingerProcessing1");
							finger.maruFingerProcessing1(eventType,
									e.x, e.y, geometry[0], geometry[1]);
							return;
						}
						else if(finger.getMultiTouchEnable() == 2) {
							logger.info("maruFingerProcessing2");
							finger.maruFingerProcessing2(eventType,
									e.x, e.y, geometry[0], geometry[1]);
							return;
						}
					}
	
					MouseEventData mouseEventData = new MouseEventData(
							MouseButtonType.LEFT.value(), eventType,
							e.x, e.y, geometry[0], geometry[1], 0);

					communicator.sendToQEMU( SendCommand.SEND_MOUSE_EVENT, mouseEventData );
				}
			}
		};

		canvas.addMouseMoveListener( canvasMouseMoveListener );

		canvasMouseListener = new MouseListener() {

			@Override
			public void mouseUp( MouseEvent e ) {
				if ( 1 == e.button ) { // left button

					int[] geometry = SkinUtil.convertMouseGeometry(e.x, e.y,
							currentState.getCurrentResolutionWidth(),
							currentState.getCurrentResolutionHeight(),
							currentState.getCurrentScale(), currentState.getCurrentAngle());
					logger.info( "mouseUp in LCD" + " x:" + geometry[0] + " y:" + geometry[1] );

					if ( true == EmulatorSkin.this.isDragStartedInLCD ) {
						EmulatorSkin.this.isDragStartedInLCD = false;
					}
					
					if(SwtUtil.isMacPlatform()) {
						if(finger.getMultiTouchEnable() == 1) {
							logger.info("maruFingerProcessing1");
							finger.maruFingerProcessing1(MouseEventType.RELEASE.value(),
									e.x, e.y, geometry[0], geometry[1]);
							return;
						}
						else if(finger.getMultiTouchEnable() == 2) {
							logger.info("maruFingerProcessing2");
							finger.maruFingerProcessing2(MouseEventType.RELEASE.value(),
									e.x, e.y, geometry[0], geometry[1]);
							return;
						}
					}
					
					MouseEventData mouseEventData = new MouseEventData(
							MouseButtonType.LEFT.value(), MouseEventType.RELEASE.value(),
							e.x, e.y, geometry[0], geometry[1], 0);

					communicator.sendToQEMU( SendCommand.SEND_MOUSE_EVENT, mouseEventData );
				} else if (2 == e.button) { // wheel button
					logger.info("wheelUp in LCD");
				}
			}

			@Override
			public void mouseDown( MouseEvent e ) {
				if ( 1 == e.button ) { // left button

					int[] geometry = SkinUtil.convertMouseGeometry(e.x, e.y,
							currentState.getCurrentResolutionWidth(),
							currentState.getCurrentResolutionHeight(),
							currentState.getCurrentScale(), currentState.getCurrentAngle());
					logger.info( "mouseDown in LCD" + " x:" + geometry[0] + " y:" + geometry[1] );

					if ( false == EmulatorSkin.this.isDragStartedInLCD ) {
						EmulatorSkin.this.isDragStartedInLCD = true;
					}
					
					if(SwtUtil.isMacPlatform()) {
						if(finger.getMultiTouchEnable() == 1) {
							logger.info("maruFingerProcessing1");
							finger.maruFingerProcessing1(MouseEventType.PRESS.value(),
									e.x, e.y, geometry[0], geometry[1]);
							return;
						}
						else if(finger.getMultiTouchEnable() == 2) {
							logger.info("maruFingerProcessing2");
							finger.maruFingerProcessing2(MouseEventType.PRESS.value(),
									e.x, e.y, geometry[0], geometry[1]);
							return;
						}
					}
		
					MouseEventData mouseEventData = new MouseEventData(
							MouseButtonType.LEFT.value(), MouseEventType.PRESS.value(),
							e.x, e.y, geometry[0], geometry[1], 0);

					communicator.sendToQEMU( SendCommand.SEND_MOUSE_EVENT, mouseEventData );
				}
			}

			@Override
			public void mouseDoubleClick( MouseEvent e ) {
				/* do nothing */
			}
		};

		canvas.addMouseListener( canvasMouseListener );

		canvasMouseWheelListener = new MouseWheelListener() {

			@Override
			public void mouseScrolled(MouseEvent e) {
				int[] geometry = SkinUtil.convertMouseGeometry(e.x, e.y,
						currentState.getCurrentResolutionWidth(),
						currentState.getCurrentResolutionHeight(),
						currentState.getCurrentScale(), currentState.getCurrentAngle());
				logger.info("mousewheel in LCD" +
						" x:" + geometry[0] + " y:" + geometry[1] + " value:" + e.count);

				int eventType;
				if (e.count < 0) {
					eventType = MouseEventType.WHEELDOWN.value();
				} else {
					eventType = MouseEventType.WHEELUP.value();
				}

				MouseEventData mouseEventData = new MouseEventData(
						MouseButtonType.WHEEL.value(), eventType,
						e.x, e.y, geometry[0], geometry[1], e.count);
				communicator.sendToQEMU(SendCommand.SEND_MOUSE_EVENT, mouseEventData);
			}
		};

		canvas.addMouseWheelListener( canvasMouseWheelListener );

		/* keyboard event */
		canvasKeyListener = new KeyListener() {

			private KeyEvent previous;
			private boolean disappearEvent = false;
			private int disappearKeycode = 0;
			private int disappearStateMask = 0;
			private int disappearKeyLocation = 0;

			@Override
			public void keyReleased( KeyEvent e ) {
				if ( logger.isLoggable( Level.INFO ) ) {
					logger.info( "'" + e.character + "':" + e.keyCode + ":" + e.stateMask + ":" + e.keyLocation );
				} else if ( logger.isLoggable( Level.FINE ) ) {
					logger.fine( e.toString() );
				}
				int keyCode = e.keyCode;
				int stateMask = e.stateMask;

				previous = null;

				/* generate a disappeared key event in Windows */
				if (SwtUtil.isWindowsPlatform() && disappearEvent) {
					disappearEvent = false;
					if (isMetaKey(e) && e.character != '\0') {
						logger.info("send previous release : keycode=" + disappearKeycode +
								", stateMask=" + disappearStateMask +
								", keyLocation=" + disappearKeyLocation);

						KeyEventData keyEventData = new KeyEventData(
								KeyEventType.RELEASED.value(),
								disappearKeycode, disappearStateMask, disappearKeyLocation);
						communicator.sendToQEMU(SendCommand.SEND_KEY_EVENT, keyEventData);

						removePressedKey(keyEventData);

						disappearKeycode = 0;
						disappearStateMask = 0;
						disappearKeyLocation = 0;
					}
				}
                else if(SwtUtil.isMacPlatform()) {
                	if(keyCode == SWT.SHIFT || keyCode == SWT.COMMAND) {
                		int tempStateMask = stateMask & ~SWT.BUTTON1;
						if(tempStateMask == (SWT.SHIFT | SWT.COMMAND)) {
							finger.setMultiTouchEnable(1);
							logger.info("enable multi-touch = mode1");
						}
						else {
							finger.setMultiTouchEnable(0);
							finger.clearFingerSlot();
							logger.info("disable multi-touch");
						}
                	}
				}

				KeyEventData keyEventData = new KeyEventData(
						KeyEventType.RELEASED.value(), keyCode, stateMask, e.keyLocation);
				communicator.sendToQEMU(SendCommand.SEND_KEY_EVENT, keyEventData);

				removePressedKey(keyEventData);
			}

			@Override
			public void keyPressed( KeyEvent e ) {
				int keyCode = e.keyCode;
				int stateMask = e.stateMask;

				/* When the pressed key event is overwritten by next key event,
				 * the release event of previous key does not occur in Windows.
				 * So, we generate a release key event by ourselves
				 * that had been disappeared. */
				if (SwtUtil.isWindowsPlatform()) {
					if (null != previous) {
						if (previous.keyCode != e.keyCode) {

							if (isMetaKey(previous)) {
								disappearEvent = true;
								disappearKeycode = keyCode;
								disappearStateMask = stateMask;
								disappearKeyLocation = e.keyLocation;
							} else {
								int previousKeyCode = previous.keyCode;
								int previousStateMask = previous.stateMask;

								if ( logger.isLoggable( Level.INFO ) ) {
									logger.info( "send previous release : '" + previous.character + "':"
											+ previous.keyCode + ":" + previous.stateMask + ":" + previous.keyLocation );
								} else if ( logger.isLoggable( Level.FINE ) ) {
									logger.fine( "send previous release :" + previous.toString() );
								}

								KeyEventData keyEventData = new KeyEventData(KeyEventType.RELEASED.value(),
										previousKeyCode, previousStateMask, previous.keyLocation);
								communicator.sendToQEMU(SendCommand.SEND_KEY_EVENT, keyEventData);

								removePressedKey(keyEventData);
							}

						}
					}
				} //end isWindowsPlatform
                else if(SwtUtil.isMacPlatform()) {
                //	if(finger.getMaxTouchPoint() > 1) {
						int tempStateMask = stateMask & ~SWT.BUTTON1;
						if((keyCode == SWT.SHIFT && (tempStateMask & SWT.COMMAND) != 0) || 
								(keyCode == SWT.COMMAND && (tempStateMask & SWT.SHIFT) != 0))
						{
							finger.setMultiTouchEnable(2);
							logger.info("enable multi-touch = mode2");
						}
						else if(keyCode == SWT.SHIFT || keyCode == SWT.COMMAND) {
							finger.setMultiTouchEnable(1);
							logger.info("enable multi-touch = mode1");
						}
                //	}
				}

				previous = e;

				if ( logger.isLoggable( Level.INFO ) ) {
					logger.info( "'" + e.character + "':" + e.keyCode + ":" + e.stateMask + ":" + e.keyLocation );
				} else if ( logger.isLoggable( Level.FINE ) ) {
					logger.fine( e.toString() );
				}

				KeyEventData keyEventData = new KeyEventData(
						KeyEventType.PRESSED.value(), keyCode, stateMask, e.keyLocation);
				communicator.sendToQEMU(SendCommand.SEND_KEY_EVENT, keyEventData);

				addPressedKey(keyEventData);
			}

		};

		canvas.addKeyListener( canvasKeyListener );

	}

	private boolean isMetaKey(KeyEvent event) {
		if (SWT.CTRL == event.keyCode ||
				SWT.ALT == event.keyCode ||
				SWT.SHIFT == event.keyCode) {
			return true;
		}
		return false;
	}

	private synchronized boolean addPressedKey(KeyEventData pressData) {
		for (KeyEventData data : pressedKeyEventList) {
			if (data.keycode == pressData.keycode &&
					data.stateMask == pressData.stateMask &&
					data.keyLocation == pressData.keyLocation) {
				return false;
			}
		}

		pressedKeyEventList.add(pressData);
		return true;
	}

	private synchronized boolean removePressedKey(KeyEventData releaseData) {

		for (KeyEventData data : pressedKeyEventList) {
			if (data.keycode == releaseData.keycode &&
					data.stateMask == releaseData.stateMask &&
					data.keyLocation == releaseData.keyLocation) {
				pressedKeyEventList.remove(data);

				return true;
			}
		}

		return false;
	}

	private void removeCanvasListeners() {
//		if ( null != canvasDragDetectListener ) {
//			lcdCanvas.removeDragDetectListener( canvasDragDetectListener );
//		}
		if (null != canvasMouseMoveListener) {
			lcdCanvas.removeMouseMoveListener(canvasMouseMoveListener);
		}

		if (null != canvasMouseListener) {
			lcdCanvas.removeMouseListener(canvasMouseListener);
		}

		if (null != canvasKeyListener) {
			lcdCanvas.removeKeyListener(canvasKeyListener);
		}

		if (null != canvasMenuDetectListener) {
			lcdCanvas.removeMenuDetectListener(canvasMenuDetectListener);
		}

		if (null != canvasFocusListener) {
			lcdCanvas.removeFocusListener(canvasFocusListener);
		}

		if (null != canvasMouseWheelListener) {
			lcdCanvas.removeMouseWheelListener(canvasMouseWheelListener);
		}
	}


	protected void openScreenShotWindow() {
		//TODO: abstract
	}

	/* toggle a key window */
	public void setIsControlPanel(boolean value) {
		isControlPanel = value;
		panelItem.setSelection(isControlPanel);
		logger.info("Select Key Window : " + isControlPanel);
	}

	public boolean getIsControlPanel() {
		return isControlPanel;
	}

	public void openKeyWindow(int attach) {
		if (controlPanel != null) {
			controlPanel.getShell().setVisible(true);
			SkinUtil.setTopMost(controlPanel.getShell(), isOnTop);
			controlPanel.setAttach(attach);

			pairTagCanvas.setVisible(true);
			return;
		}

		/* create a key window */
		List<KeyMapType> keyMapList =
				SkinUtil.getHWKeyMapList(currentState.getCurrentRotationId());

		if (keyMapList == null) {
			logger.info("keyMapList is null");
			return;
		} else if (keyMapList.isEmpty() == true) {
			logger.info("keyMapList is empty");
			return;
		}

		try {
			controlPanel = new ControlPanel(shell, colorPairTag,
					communicator, keyMapList);
			SkinUtil.setTopMost(controlPanel.getShell(), isOnTop);

			//colorPairTag = controlPanel.getPairTagColor();
			pairTagCanvas.setVisible(true);

			controlPanel.open();
			/* do not add at this line */
		} finally {
			controlPanel = null;
		}
	}

	public void hideKeyWindow() {
		controlPanel.getShell().setVisible(false);
		pairTagCanvas.setVisible(false);
	}

	private void addMenuItems(final Shell shell, final Menu menu) {

		/* Emulator detail info menu */
		final MenuItem detailInfoItem = new MenuItem(menu, SWT.PUSH);

		String emulatorName = SkinUtil.makeEmulatorName(config);
		detailInfoItem.setText(emulatorName);
		detailInfoItem.setImage(imageRegistry.getIcon(IconName.DETAIL_INFO));
		detailInfoItem.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e) {
				if (logger.isLoggable(Level.FINE)) {
					logger.fine("Open detail info");
				}

				String emulatorName = SkinUtil.makeEmulatorName(config);
				DetailInfoDialog detailInfoDialog = new DetailInfoDialog(
						shell, emulatorName, communicator, config);
				detailInfoDialog.open();
			}
		} );

		new MenuItem(menu, SWT.SEPARATOR);

		/* Always on top menu */
		if (!SwtUtil.isMacPlatform()) { /* not supported on mac */
			final MenuItem onTopItem = new MenuItem(menu, SWT.CHECK);
			onTopItem.setText("&Always On Top");
			onTopItem.setSelection(isOnTop);

			onTopItem.addSelectionListener(new SelectionAdapter() {
				@Override
				public void widgetSelected(SelectionEvent e) {
					isOnTop = onTopItem.getSelection();

					logger.info("Select Always On Top : " + isOnTop);
					// readyToReopen(EmulatorSkin.this, isOnTop);

					if (SkinUtil.setTopMost(shell, isOnTop) == false) {
						logger.info("failed to Always On Top");
					} else {
						if (controlPanel != null) {
							SkinUtil.setTopMost(controlPanel.getShell(), isOnTop);
						}
					}
				}
			} );
		}

		/* Rotate menu */
		final MenuItem rotateItem = new MenuItem(menu, SWT.CASCADE);
		rotateItem.setText("&Rotate");
		rotateItem.setImage(imageRegistry.getIcon(IconName.ROTATE));
		Menu rotateMenu = createRotateMenu(menu.getShell());
		rotateItem.setMenu(rotateMenu);

		/* Scale menu */
		final MenuItem scaleItem = new MenuItem(menu, SWT.CASCADE);
		scaleItem.setText("&Scale");
		scaleItem.setImage(imageRegistry.getIcon(IconName.SCALE));
		Menu scaleMenu = createScaleMenu(menu.getShell());
		scaleItem.setMenu(scaleMenu);

		new MenuItem(menu, SWT.SEPARATOR);

		/* Key Window menu */
		if (skinInfo.isPhoneShape() == false) { //TODO:
		panelItem = new MenuItem(menu, SWT.CHECK);
		panelItem.setText("&Key Window");
		panelItem.setSelection(isControlPanel);

		panelItem.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e) {
				final boolean isControlPanel = panelItem.getSelection();

				setIsControlPanel(isControlPanel);
				if (isControlPanel == true) {
					openKeyWindow((controlPanel == null) ?
							SWT.NONE : controlPanel.isAttach());
				} else { /* hide a key window */
					if (controlPanel != null &&
							controlPanel.isAttach() != SWT.NONE) {
						pairTagCanvas.setVisible(false);
						controlPanel.getShell().close();
					} else {
						hideKeyWindow();
					}
				}
			}
		} );
		}

		/* Advanced menu */
		final MenuItem advancedItem = new MenuItem( menu, SWT.CASCADE );
		advancedItem.setText( "Ad&vanced" );
		advancedItem.setImage( imageRegistry.getIcon( IconName.ADVANCED ) );
		Menu advancedMenu = createAdvancedMenu( menu.getShell() );
		advancedItem.setMenu( advancedMenu );

		/* Shell menu */
		final MenuItem shellItem = new MenuItem( menu, SWT.PUSH );
		shellItem.setText( "S&hell" );
		shellItem.setImage( imageRegistry.getIcon( IconName.SHELL ) );
		
		shellItem.addSelectionListener( new SelectionAdapter() {
			@Override
			public void widgetSelected( SelectionEvent e ) {
				if ( !communicator.isSensorDaemonStarted() ) {
					SkinUtil.openMessage(shell, null,
							"SDB is not ready.\nPlease wait until the emulator is completely boot up.",
							SWT.ICON_WARNING, config);
					return;
				}

				String sdbPath = SkinUtil.getSdbPath();

				File sdbFile = new File(sdbPath);
				if (!sdbFile.exists()) {
					logger.log( Level.INFO, "SDB file is not exist : " + sdbFile.getAbsolutePath());
					try {
						SkinUtil.openMessage( shell, null,
								"SDB file is not exist in the following path.\n" + sdbFile.getCanonicalPath()
								, SWT.ICON_ERROR, config );
					} catch (IOException ee) {
						logger.log( Level.SEVERE, ee.getMessage(), ee );
					}
					return;
				}

				int portSdb = config.getArgInt( ArgsConstants.NET_BASE_PORT );

				ProcessBuilder procSdb = new ProcessBuilder();

				if (SwtUtil.isLinuxPlatform()) {
					procSdb.command("/usr/bin/gnome-terminal", "--disable-factory",
							"--title=" + SkinUtil.makeEmulatorName( config ), "-x", sdbPath,
							"-s", "emulator-" + portSdb, "shell");
				} else if (SwtUtil.isWindowsPlatform()) {
					procSdb.command("cmd.exe", "/c", "start", sdbPath, "sdb",
							"-s", "emulator-" + portSdb, "shell");
				} else if (SwtUtil.isMacPlatform()) {
					procSdb.command("/usr/X11/bin/uxterm", "-T", "emulator-" + portSdb, "-e", sdbPath,"shell");
				}
				logger.log(Level.INFO, procSdb.command().toString());
			

				try {
					procSdb.start(); // open sdb shell
				} catch ( Exception ee ) {
					logger.log( Level.SEVERE, ee.getMessage(), ee );
					SkinUtil.openMessage( shell, null, "Fail to open Shell: \n" + ee.getMessage(), SWT.ICON_ERROR, config );
				}

				communicator.sendToQEMU( SendCommand.OPEN_SHELL, null );
			}
		} );

		new MenuItem( menu, SWT.SEPARATOR );

		/* Close menu */
		MenuItem closeItem = new MenuItem( menu, SWT.PUSH );
		closeItem.setText( "&Close" );
		closeItem.setImage( imageRegistry.getIcon( IconName.CLOSE ) );
		closeItem.addSelectionListener( new SelectionAdapter() {
			@Override
			public void widgetSelected( SelectionEvent e ) {
				logger.info("Close Menu is selected");
				communicator.sendToQEMU( SendCommand.CLOSE, null );
			}
		} );

	}

	private Menu createRotateMenu( final Shell shell ) {

		Menu menu = new Menu( shell, SWT.DROP_DOWN );

		final List<MenuItem> rotationList = new ArrayList<MenuItem>();

		Iterator<Entry<Short, RotationType>> iterator = SkinRotation.getRotationIterator();

		while ( iterator.hasNext() ) {

			Entry<Short, RotationType> entry = iterator.next();
			Short rotationId = entry.getKey();
			RotationType section = entry.getValue();

			final MenuItem menuItem = new MenuItem( menu, SWT.RADIO );
			menuItem.setText( section.getName().value() );
			menuItem.setData( rotationId );

			if (currentState.getCurrentRotationId() == rotationId) {
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

					/* roll back a selection */
					item.setSelection( false );

					for ( MenuItem m : rotationList ) {
						short rotationId = (Short) m.getData();
						if (currentState.getCurrentRotationId() == rotationId) {
							m.setSelection( true );
							break;
						}
					}

					SkinUtil.openMessage(shell, null,
							"Rotation is not ready.\nPlease wait until the emulator is completely boot up.",
							SWT.ICON_WARNING, config);
					return;
				}

				short rotationId = ((Short) item.getData());

				skinComposer.arrangeSkin(currentState.getCurrentScale(), rotationId);

				LcdStateData lcdStateData =
						new LcdStateData(currentState.getCurrentScale(), rotationId);
				communicator.sendToQEMU(SendCommand.CHANGE_LCD_STATE, lcdStateData);
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
		scaleOneItem.setData( Scale.SCALE_100 );
		scaleList.add( scaleOneItem );

		final MenuItem scaleThreeQtrItem = new MenuItem( menu, SWT.RADIO );
		scaleThreeQtrItem.setText( "3/4x" );
		scaleThreeQtrItem.setData( Scale.SCALE_75 );
		scaleList.add( scaleThreeQtrItem );

		final MenuItem scalehalfItem = new MenuItem( menu, SWT.RADIO );
		scalehalfItem.setText( "1/2x" );
		scalehalfItem.setData( Scale.SCALE_50 );
		scaleList.add( scalehalfItem );

		final MenuItem scaleOneQtrItem = new MenuItem( menu, SWT.RADIO );
		scaleOneQtrItem.setText( "1/4x" );
		scaleOneQtrItem.setData( Scale.SCALE_25 );
		scaleList.add( scaleOneQtrItem );

		SelectionAdapter selectionAdapter = new SelectionAdapter() {

			@Override
			public void widgetSelected( SelectionEvent e ) {

				MenuItem item = (MenuItem) e.getSource();

				boolean selection = item.getSelection();

				if ( !selection ) {
					return;
				}

				int scale = ((Scale) item.getData()).value();

				skinComposer.arrangeSkin(scale, currentState.getCurrentRotationId());

				LcdStateData lcdStateData =
						new LcdStateData(scale, currentState.getCurrentRotationId());
				communicator.sendToQEMU(SendCommand.CHANGE_LCD_STATE, lcdStateData);

			}
		};
		
		for ( MenuItem menuItem : scaleList ) {

			int scale = ( (Scale) menuItem.getData() ).value();
			if (currentState.getCurrentScale() == scale) {
				menuItem.setSelection( true );
			}

			menuItem.addSelectionListener( selectionAdapter );

		}

		return menu;

	}

	private Menu createDiagnosisMenu(Shell shell) {
		Menu menu = new Menu(shell, SWT.DROP_DOWN);

		final MenuItem ramdumpItem = new MenuItem(menu, SWT.PUSH);
		ramdumpItem.setText("&Ram Dump");

		ramdumpItem.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e) {
				logger.info("Ram dump menu is selected");

				communicator.setRamdumpFlag(true);
				communicator.sendToQEMU(SendCommand.RAM_DUMP, null);

				RamdumpDialog ramdumpDialog;
				try {
					ramdumpDialog = new RamdumpDialog(EmulatorSkin.this.shell, communicator, config);
					ramdumpDialog.open();
				} catch (IOException ee) {
					logger.log( Level.SEVERE, ee.getMessage(), ee);
				}
			}
		});

		final MenuItem guestdumpItem = new MenuItem(menu, SWT.PUSH);
		guestdumpItem.setText("&Guest Memory Dump");

		guestdumpItem.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e) {
				logger.info("Guest memory dump menu is selected");

				communicator.setRamdumpFlag(true);
				communicator.sendToQEMU(SendCommand.GUEST_DUMP, null);
			}
		});

		return menu;
	}

	private Menu createAdvancedMenu(final Shell shell) {

		final Menu menu = new Menu(shell, SWT.DROP_DOWN);

		/* Screen shot menu */
		final MenuItem screenshotItem = new MenuItem(menu, SWT.PUSH);
		screenshotItem.setText("&Screen Shot");
		screenshotItem.setImage(imageRegistry.getIcon(IconName.SCREENSHOT));
		screenshotItem.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e) {
				logger.info("ScreenShot Menu is selected");

				openScreenShotWindow();
			}
		} );

		// VirtIO Keyboard Menu
		final MenuItem hostKeyboardItem = new MenuItem(menu, SWT.CASCADE);
		hostKeyboardItem.setText("&Host Keyboard");
		hostKeyboardItem.setImage(imageRegistry.getIcon(IconName.HOST_KEYBOARD));

		Menu hostKeyboardMenu = new Menu(shell, SWT.DROP_DOWN);

		final MenuItem kbdOnItem = new MenuItem(hostKeyboardMenu, SWT.RADIO);
		kbdOnItem.setText("On");
		kbdOnItem.setSelection( isOnKbd );

		final MenuItem kbdOffItem = new MenuItem(hostKeyboardMenu, SWT.RADIO);
		kbdOffItem.setText("Off");
		kbdOffItem.setSelection(!isOnKbd);

		SelectionAdapter kbdSelectionAdaptor = new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e) {
				if (!communicator.isSensorDaemonStarted()) {
					SkinUtil.openMessage(shell, null,
							"Host Keyboard is not ready.\nPlease wait until the emulator is completely boot up.",
							SWT.ICON_WARNING, config);
					kbdOnItem.setSelection(isOnKbd);
					kbdOffItem.setSelection(!isOnKbd);

					return;
				}

				MenuItem item = (MenuItem) e.getSource();
				if (item.getSelection()) {
					boolean on = item.equals(kbdOnItem);
					isOnKbd = on;
					logger.info("Host Keyboard " + isOnKbd);

					communicator.sendToQEMU(
							SendCommand.HOST_KBD, new BooleanData(on, SendCommand.HOST_KBD.toString()));
				}

			}
		};

		kbdOnItem.addSelectionListener(kbdSelectionAdaptor);
		kbdOffItem.addSelectionListener(kbdSelectionAdaptor);

		hostKeyboardItem.setMenu(hostKeyboardMenu);

		/* Diagnosis menu */
		if (SwtUtil.isLinuxPlatform()) { //TODO: windows
			final MenuItem diagnosisItem = new MenuItem(menu, SWT.CASCADE);
			diagnosisItem.setText("&Diagnosis");
			diagnosisItem.setImage(imageRegistry.getIcon(IconName.DIAGNOSIS));
			Menu diagnosisMenu = createDiagnosisMenu(menu.getShell());
			diagnosisItem.setMenu(diagnosisMenu);
		}

		new MenuItem(menu, SWT.SEPARATOR);

		/* About menu */
		final MenuItem aboutItem = new MenuItem(menu, SWT.PUSH);
		aboutItem.setText("&About");
		aboutItem.setImage(imageRegistry.getIcon(IconName.ABOUT));

		aboutItem.addSelectionListener(new SelectionAdapter() {
			private boolean isOpen;

			@Override
			public void widgetSelected(SelectionEvent e) {
				if (!isOpen) {
					isOpen = true;

					logger.info("Open the about dialog");
					AboutDialog dialog = new AboutDialog(shell);
					dialog.open();
					isOpen = false;
				}
			}
		} );

		new MenuItem(menu, SWT.SEPARATOR);

		/* Force close menu */
		final MenuItem forceCloseItem = new MenuItem(menu, SWT.PUSH);
		forceCloseItem.setText("&Force Close");
		forceCloseItem.setImage(imageRegistry.getIcon(IconName.FORCE_CLOSE));
		forceCloseItem.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e) {
				logger.info("Force close is selected");

				int answer = SkinUtil.openMessage(shell, null,
						"If you force stop an emulator, it may cause some problems.\n" +
						"Are you sure you want to contiue?",
						SWT.ICON_QUESTION | SWT.OK | SWT.CANCEL, config);

				if (answer == SWT.OK) {
					logger.info("force close!!!");
					System.exit(-1);
				}
			}
		});

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

	public short getCurrentRotationId() {
		return currentState.getCurrentRotationId();
	}

}
