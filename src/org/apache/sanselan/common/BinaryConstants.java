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
package org.apache.sanselan.common;

public interface BinaryConstants
{
	public static final int BYTE_ORDER_INTEL = 'I';
	public static final int BYTE_ORDER_LEAST_SIGNIFICANT_BYTE = BYTE_ORDER_INTEL;
	public static final int BYTE_ORDER_LSB = BYTE_ORDER_INTEL;
	public static final int BYTE_ORDER_LITTLE_ENDIAN = BYTE_ORDER_INTEL;

	public static final int BYTE_ORDER_MOTOROLA = 'M';
	public static final int BYTE_ORDER_MOST_SIGNIFICANT_BYTE = BYTE_ORDER_MOTOROLA;
	public static final int BYTE_ORDER_MSB = BYTE_ORDER_MOTOROLA;
	public static final int BYTE_ORDER_NETWORK = BYTE_ORDER_MOTOROLA;
	public static final int BYTE_ORDER_BIG_ENDIAN = BYTE_ORDER_MOTOROLA;

}