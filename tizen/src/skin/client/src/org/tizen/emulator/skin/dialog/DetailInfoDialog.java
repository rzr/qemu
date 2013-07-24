/**
 * Display the emulator detail information
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

import java.io.IOException;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.Map.Entry;
import java.util.logging.Level;
import java.util.logging.Logger;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.program.Program;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.swt.widgets.TableItem;
import org.tizen.emulator.skin.comm.ICommunicator.SendCommand;
import org.tizen.emulator.skin.comm.sock.SocketCommunicator;
import org.tizen.emulator.skin.comm.sock.SocketCommunicator.DataTranfer;
import org.tizen.emulator.skin.config.EmulatorConfig;
import org.tizen.emulator.skin.config.EmulatorConfig.ArgsConstants;
import org.tizen.emulator.skin.info.SkinInformation;
import org.tizen.emulator.skin.log.SkinLogger;
import org.tizen.emulator.skin.util.SkinUtil;
import org.tizen.emulator.skin.util.StringUtil;
import org.tizen.emulator.skin.util.SwtUtil;

/**
 * 
 *
 */
public class DetailInfoDialog extends SkinDialog {
	public final static String DATA_DELIMITER = "#";

	public final static String KEY_VM_NAME = "VM Name";
	public final static String KEY_SKIN_NAME = "Skin Name";
	public final static String KEY_CPU_ARCH = "CPU";
	public final static String KEY_RAM_SIZE = "RAM Size";
	public final static String KEY_DISPLAY_RESOLUTION = "Display Resolution";
	public final static String KEY_DISPLAY_DENSITY = "Display Density";
	public final static String KEY_FILESHARING = "File Sharing";
	public final static String KEY_FILESHARED_PATH = "File Shared Path";
	public final static String KEY_CPU_VIRTUALIZATION = "CPU Virtualization";
	public final static String KEY_GPU_VIRTUALIZATION = "GPU Virtualization";
	public final static String KEY_IMAGE_PATH = "Image Path";
	public final static String KEY_LOG_PATH = "Log Path";

	public final static String VALUE_NONE = "None";
	public final static String VALUE_SUPPORTED = "Supported";
	public final static String VALUE_NOTSUPPORTED = "Not Supported";
	public final static String VALUE_ENABLED = "Enabled";
	public final static String VALUE_DISABLED = "Disabled";

	private Logger logger =
			SkinLogger.getSkinLogger(DetailInfoDialog.class).getLogger();

	private SocketCommunicator communicator;
	private EmulatorConfig config;
	private SkinInformation skinInfo;
	private Table table;
	private LinkedHashMap<String, String> refinedData;

	/**
	 *  Constructor
	 */
	public DetailInfoDialog(Shell parent, String emulatorName,
			SocketCommunicator communicator, EmulatorConfig config, SkinInformation skinInfo) {
		super(parent, "Detail Info" + " - " + emulatorName,
				SWT.DIALOG_TRIM | SWT.APPLICATION_MODAL | SWT.RESIZE | SWT.MAX);

		this.communicator = communicator;
		this.config = config;
		this.skinInfo = skinInfo;
	}

	@Override
	protected Composite createArea(Composite parent) {
		String infoData = queryData();
		if (StringUtil.isEmpty(infoData)) {
			return null;
		}

		Composite composite = new Composite(parent, SWT.NONE);
		composite.setLayout(new FillLayout());

		table = new Table(composite,
				SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
		table.setHeaderVisible(true);
		table.setLinesVisible(true);

		TableColumn[] column = new TableColumn[2];

		column[0] = new TableColumn(table, SWT.LEFT);
		column[0].setText("Feature");

		column[1] = new TableColumn(table, SWT.LEFT);
		column[1].setText("Value");

		int index = 0;

		refinedData = composeAndParseData(infoData);
		Iterator<Entry<String, String>> iterator =
				refinedData.entrySet().iterator();

		while (iterator.hasNext()) {
			Entry<String, String> entry = iterator.next();

			TableItem tableItem = new TableItem(table, SWT.NONE, index);
			tableItem.setText(new String[] { entry.getKey(), entry.getValue() });
			index++;
		}

		column[0].pack();
		column[1].pack();
		table.pack();

		/* browse the appropriate path when item is double clicked */
		table.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent event) {
				/* do nothing */
			}

			@Override
			public void widgetDefaultSelected(SelectionEvent event) {
				if (table.getSelectionCount() > 1) {
					return;
				}

				TableItem tableItem = ((TableItem)table.getSelection()[0]);
				String openPath = VALUE_NONE;

				if (tableItem.getText().compareTo(KEY_LOG_PATH) == 0) {
					openPath = refinedData.get(KEY_LOG_PATH);
				} else if (tableItem.getText().compareTo(KEY_IMAGE_PATH) == 0) {
					openPath = refinedData.get(KEY_IMAGE_PATH);
				} else if (tableItem.getText().compareTo(KEY_FILESHARED_PATH) == 0) {
					openPath = refinedData.get(KEY_FILESHARED_PATH);
				}

				try {
					openPath = StringUtil.getCanonicalPath(openPath);
				} catch (IOException e) {
					logger.warning("Invalid path");
				}

				if (openPath.compareTo(VALUE_NONE) == 0 ||
						openPath.compareTo("") == 0) {
					return;
				}

				Program.launch(openPath);

				/*ProcessBuilder procBrowser = new ProcessBuilder();

				if (SwtUtil.isLinuxPlatform()) {
					procBrowser.command("nautilus", "--browser", openPath);
				} else if (SwtUtil.isWindowsPlatform()) {
					procBrowser.command("explorer", "\"" + openPath + "\"");
				} else if (SwtUtil.isMacPlatform()) {
					logger.warning("not supported yet");
				}

				if (procBrowser.command().isEmpty() == false) {
					try {
						procBrowser.start();
					} catch (Exception e) {
						logger.log( Level.SEVERE, e.getMessage(), e);
					}
				}*/

			}
		});

		return composite;
	}

	@Override
	protected void setShellSize() {
		if (SwtUtil.isLinuxPlatform()) {
			shell.setSize((int) (402 * 1.618/* golden ratio */), 402);
		} else {
			shell.setSize((int) (372 * 1.618), 372);
		}
	}

	private String queryData() {
		String infoData = null;

		DataTranfer dataTranfer =
				communicator.sendDataToQEMU(SendCommand.DETAIL_INFO, null, true);
		byte[] receivedData = communicator.getReceivedData(dataTranfer);

		if (null != receivedData) {
			infoData = new String(receivedData);
		} else {
			logger.severe("Fail to get detail info");
			SkinUtil.openMessage(shell, null,
					"Fail to get detail info", SWT.ICON_ERROR, config);
		}

		return infoData;
	}

	private LinkedHashMap<String, String> composeAndParseData(String infoData) {
		logger.info("Received infoData : " + infoData);

		String cpu = "";
		String ramSize = "";
		String dpi = "";
		String imagePath = "";
		String sharedPath = "";
		boolean isCpuVirtual = false;
		boolean isGpuVirtual = false;
		String cpuVirtualCompare = "";
		String gpuVirtualCompare = "";
		String logPath = "";
		boolean isHaxError = false;

		if (SwtUtil.isLinuxPlatform()) {
			cpuVirtualCompare = "-enable-kvm";
		} else if (SwtUtil.isWindowsPlatform()) {
			cpuVirtualCompare = "-enable-hax";
		}
		gpuVirtualCompare = "-enable-gl";

		String[] split = infoData.split(DATA_DELIMITER);

		for (int i = 0; i < split.length; i++) {
			if (0 == i) {
				String exec = split[i].trim().toLowerCase();
				if (SwtUtil.isWindowsPlatform()) {
					if (4 <= exec.length()) {
						/* remove '.exe' in Windows */
						exec = exec.substring(0, exec.length() - 4);
					}
				}

				// TODO:
				if (exec.endsWith("x86")) {
					cpu = "x86";
				} else if (exec.endsWith("arm")) {
					cpu = "ARM";
				}
			} else {
				if (i + 1 <= split.length) {
					String arg = split[i].trim();

					if ("-m".equals(arg))
					{
						ramSize = split[i + 1].trim();
					}
					else if ("-drive".equals(arg))
					{
						/* arg : file=/path/emulimg.x86,... */
						arg = split[i + 1].trim();

						if (arg.startsWith("file=")) {
							String[] sp = arg.split(",");
							String[] sp2 = sp[0].split("=");
							String drivePath = sp2[sp2.length - 1];

							imagePath = drivePath;
						}
					}
					else if ("-virtfs".equals(arg))
					{
						/* arg : local,path=/path,... */
						arg = split[i + 1].trim();
						String[] sp = arg.split(",");

						if (1 < sp.length) {
							int spIndex = sp[1].indexOf("=");
							sharedPath = sp[1].substring(spIndex + 1, sp[1].length());
						}
					}
					else if ("-append".equals(arg))
					{
						arg = split[i + 1].trim();
						String[] splitSub = arg.split(" ");

						for (int j = 0; j < splitSub.length; j++) {
							String parameterKernel = splitSub[j].trim();

							if (parameterKernel.startsWith("dpi=")) {
								String[] sp = parameterKernel.split("=");
								if (1 < sp.length) {
									dpi = Integer.toString(
											Integer.parseInt(sp[1]) / 10);
								}
							}
						}
					}
					else if (cpuVirtualCompare.equals(arg))
					{
						isCpuVirtual = true;
					}
					else if (gpuVirtualCompare.equals(arg))
					{
						isGpuVirtual = true;
					}
					else if (arg.startsWith("hax_error="))
					{
						String[] sp = arg.split("=");
						if (1 < sp.length) {
							isHaxError = Boolean.parseBoolean(sp[1]);
						}
					}
					else if (arg.startsWith("log_path="))
					{
						String[] sp = arg.split("=");
						if (1 < sp.length) {
							logPath = sp[1];

							try {
								logPath = StringUtil.getCanonicalPath(logPath);
							} catch (IOException e) {
								logger.log(Level.SEVERE, e.getMessage(), e);
							}

							logger.info("log path = " + logPath); /* without filename */
						}
					}

				}
			}
		}

		LinkedHashMap<String, String> result =
				new LinkedHashMap<String, String>();

		/* VM name */
		result.put(KEY_VM_NAME, SkinUtil.getVmName(config));

		/* Skin name */
		result.put(KEY_SKIN_NAME, skinInfo.getSkinName());

		/* CPU srchitecture */
		result.put(KEY_CPU_ARCH, cpu);

		/* RAM size */
		result.put(KEY_RAM_SIZE, ramSize);

		/* Target display resolution */
		int width = config.getArgInt(ArgsConstants.RESOLUTION_WIDTH);
		int height = config.getArgInt(ArgsConstants.RESOLUTION_HEIGHT);
		result.put(KEY_DISPLAY_RESOLUTION, width + " x " + height);

		/* DPI (dots per inch) */
		result.put(KEY_DISPLAY_DENSITY, dpi);

		/* Whether host file sharing is supported */
		if (StringUtil.isEmpty(sharedPath)) {
			result.put(KEY_FILESHARING, VALUE_NOTSUPPORTED);
			result.put(KEY_FILESHARED_PATH, VALUE_NONE);
		} else {
			result.put(KEY_FILESHARING, VALUE_SUPPORTED);
			result.put(KEY_FILESHARED_PATH, sharedPath);
		}

		/* Whether hardware virtualization is supported */
		if (isCpuVirtual == true) {
			if (isHaxError == true) {
				result.put(KEY_CPU_VIRTUALIZATION,
						"Disable(insufficient memory for driver)");
			} else {
				result.put(KEY_CPU_VIRTUALIZATION, VALUE_ENABLED);
			}
		} else {
			result.put(KEY_CPU_VIRTUALIZATION, VALUE_DISABLED);
		}

		if (isGpuVirtual == true) {
			result.put(KEY_GPU_VIRTUALIZATION, VALUE_ENABLED);
		} else {
			result.put(KEY_GPU_VIRTUALIZATION, VALUE_DISABLED);
		}

		/* platform image path */
		if (StringUtil.isEmpty(imagePath)) {
			result.put(KEY_IMAGE_PATH, "Not identified");
		} else {
			result.put(KEY_IMAGE_PATH, imagePath);
		}

		/* emulator log path */
		if (StringUtil.isEmpty(logPath)) {
			result.put(KEY_LOG_PATH, VALUE_NONE);
		} else {
			result.put(KEY_LOG_PATH, logPath);
		}

		return result;
	}

	@Override
	protected void createButtons(Composite parent) {
		super.createButtons(parent);

		Composite composite = new Composite(parent, SWT.NONE);
		FillLayout fillLayout = new FillLayout(SWT.HORIZONTAL);
		composite.setLayout(fillLayout);

		createOKButton(composite, true);
	};

	@Override
	protected void close() {
		logger.info("close the detail info dialog");
	}
}
