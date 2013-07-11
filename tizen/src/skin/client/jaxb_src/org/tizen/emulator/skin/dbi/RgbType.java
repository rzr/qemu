//
/**
 * XML Binding
 *
 * Copyright (C) 2011 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
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

// This file was generated by the JavaTM Architecture for XML Binding(JAXB) Reference Implementation, v2.2.7 
// See <a href="http://java.sun.com/xml/jaxb">http://java.sun.com/xml/jaxb</a> 
// Any modifications to this file will be lost upon recompilation of the source schema. 
// Generated on: 2013.07.11 at 12:12:25 PM KST 
//


package org.tizen.emulator.skin.dbi;

import javax.xml.bind.annotation.XmlAccessType;
import javax.xml.bind.annotation.XmlAccessorType;
import javax.xml.bind.annotation.XmlAttribute;
import javax.xml.bind.annotation.XmlSchemaType;
import javax.xml.bind.annotation.XmlType;


/**
 * <p>Java class for rgbType complex type.
 * 
 * <p>The following schema fragment specifies the expected content contained within this class.
 * 
 * <pre>
 * &lt;complexType name="rgbType">
 *   &lt;complexContent>
 *     &lt;restriction base="{http://www.w3.org/2001/XMLSchema}anyType">
 *       &lt;attribute name="R" type="{http://www.w3.org/2001/XMLSchema}unsignedInt" />
 *       &lt;attribute name="G" type="{http://www.w3.org/2001/XMLSchema}unsignedInt" />
 *       &lt;attribute name="B" type="{http://www.w3.org/2001/XMLSchema}unsignedInt" />
 *     &lt;/restriction>
 *   &lt;/complexContent>
 * &lt;/complexType>
 * </pre>
 * 
 * 
 */
@XmlAccessorType(XmlAccessType.FIELD)
@XmlType(name = "rgbType")
public class RgbType {

    @XmlAttribute(name = "R")
    @XmlSchemaType(name = "unsignedInt")
    protected Long r;
    @XmlAttribute(name = "G")
    @XmlSchemaType(name = "unsignedInt")
    protected Long g;
    @XmlAttribute(name = "B")
    @XmlSchemaType(name = "unsignedInt")
    protected Long b;

    /**
     * Gets the value of the r property.
     * 
     * @return
     *     possible object is
     *     {@link Long }
     *     
     */
    public Long getR() {
        return r;
    }

    /**
     * Sets the value of the r property.
     * 
     * @param value
     *     allowed object is
     *     {@link Long }
     *     
     */
    public void setR(Long value) {
        this.r = value;
    }

    /**
     * Gets the value of the g property.
     * 
     * @return
     *     possible object is
     *     {@link Long }
     *     
     */
    public Long getG() {
        return g;
    }

    /**
     * Sets the value of the g property.
     * 
     * @param value
     *     allowed object is
     *     {@link Long }
     *     
     */
    public void setG(Long value) {
        this.g = value;
    }

    /**
     * Gets the value of the b property.
     * 
     * @return
     *     possible object is
     *     {@link Long }
     *     
     */
    public Long getB() {
        return b;
    }

    /**
     * Sets the value of the b property.
     * 
     * @param value
     *     allowed object is
     *     {@link Long }
     *     
     */
    public void setB(Long value) {
        this.b = value;
    }

}
