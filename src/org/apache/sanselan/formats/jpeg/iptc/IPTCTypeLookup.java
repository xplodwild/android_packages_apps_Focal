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
package org.apache.sanselan.formats.jpeg.iptc;

import java.util.HashMap;
import java.util.Map;

public abstract class IPTCTypeLookup implements IPTCConstants
{

	private static final Map IPTC_TYPE_MAP = new HashMap();
	static
	{
		for (int i = 0; i < IPTC_TYPES.length; i++)
		{
			IPTCType iptcType = IPTC_TYPES[i];
			Integer key = new Integer(iptcType.type);
			IPTC_TYPE_MAP.put(key, iptcType);
		}
	}

	public static final IPTCType getIptcType(int type)
	{
		Integer key = new Integer(type);
		if (!IPTC_TYPE_MAP.containsKey(key))
			return IPTCType.getUnknown(type);
		return (IPTCType) IPTC_TYPE_MAP.get(key);
	}
}