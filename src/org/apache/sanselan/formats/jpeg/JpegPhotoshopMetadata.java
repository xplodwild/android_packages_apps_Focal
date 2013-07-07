/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.apache.sanselan.formats.jpeg;

import java.util.Collections;
import java.util.List;

import org.apache.sanselan.common.ImageMetadata;
import org.apache.sanselan.formats.jpeg.iptc.IPTCConstants;
import org.apache.sanselan.formats.jpeg.iptc.IPTCRecord;
import org.apache.sanselan.formats.jpeg.iptc.PhotoshopApp13Data;
import org.apache.sanselan.util.Debug;

public class JpegPhotoshopMetadata extends ImageMetadata implements
		IPTCConstants
{

	public final PhotoshopApp13Data photoshopApp13Data;

	public JpegPhotoshopMetadata(final PhotoshopApp13Data photoshopApp13Data)
	{
		this.photoshopApp13Data = photoshopApp13Data;

		List records = photoshopApp13Data.getRecords();
		Collections.sort(records, IPTCRecord.COMPARATOR);
		for (int j = 0; j < records.size(); j++)
		{
			IPTCRecord element = (IPTCRecord) records.get(j);
			if (element.iptcType.type != IPTC_TYPE_RECORD_VERSION.type)
				add(element.getIptcTypeName(), element.getValue());
		}
	}

	public void dump()
	{
		Debug.debug(this.toString());
	}

}