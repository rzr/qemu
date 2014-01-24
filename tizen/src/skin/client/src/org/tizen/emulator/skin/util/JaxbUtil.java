/**
 * JAXB Utilities
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

package org.tizen.emulator.skin.util;

import java.io.ByteArrayInputStream;
import java.io.InputStream;
import java.io.OutputStream;

import javax.xml.bind.JAXBContext;
import javax.xml.bind.JAXBElement;
import javax.xml.bind.JAXBException;
import javax.xml.bind.Marshaller;
import javax.xml.bind.Unmarshaller;
import javax.xml.transform.stream.StreamSource;

import org.tizen.emulator.skin.exception.JaxbException;


/**
 * 
 *
 */
public class JaxbUtil {
	private JaxbUtil() {
		/* do nothing */
	}

	public static <T> T unmarshal(
			byte[] inputBytes, Class<T> clazz) throws JaxbException {
		return unmarshal(new ByteArrayInputStream(inputBytes), clazz);
	}

	public static <T> T unmarshal(
			InputStream in, Class<T> clazz) throws JaxbException {
		try {
			Unmarshaller u = getContext(clazz).createUnmarshaller();
			JAXBElement<T> element = u.unmarshal(new StreamSource(in), clazz);

			return element.getValue();
		} catch (JAXBException e) {
			throw new JaxbException(e);
		} catch (Throwable e) {
			throw new JaxbException(e);
		}
	}

	public static void marshal(
			Object object, OutputStream out) throws JaxbException {
		try {
			Marshaller m = getContext(object.getClass()).createMarshaller();

			m.setProperty(Marshaller.JAXB_ENCODING, "UTF-8");
			m.setProperty(Marshaller.JAXB_FORMATTED_OUTPUT, Boolean.TRUE);

			m.marshal(object, out);
		} catch (JAXBException e) {
			throw new JaxbException(e);
		} catch (Throwable e) {
			throw new JaxbException(e);
		}
	}

	private static JAXBContext getContext(Class<?> clazz) throws JaxbException {
		try {
			String qualifier = clazz.getCanonicalName();
			if (qualifier == null) {
				throw new JAXBException("underlying class does not have a canonical name");
			}

			int index = qualifier.lastIndexOf('.');
			if (-1 != index) {
				qualifier = qualifier.substring(0, index);
			}

			return JAXBContext.newInstance(qualifier, clazz.getClassLoader());
		} catch (JAXBException e) {
			throw new JaxbException(e);
		}
	}
}
