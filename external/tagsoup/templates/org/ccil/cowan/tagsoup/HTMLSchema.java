// This file is part of TagSoup and is Copyright 2002-2008 by John Cowan.
//
// TagSoup is licensed under the Apache License,
// Version 2.0.  You may obtain a copy of this license at
// http://www.apache.org/licenses/LICENSE-2.0 .  You may also have
// additional legal rights not granted by this license.
//
// TagSoup is distributed in the hope that it will be useful, but
// unless required by applicable law or agreed to in writing, TagSoup
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
// OF ANY KIND, either express or implied; not even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// 
// 
/**
This class provides a Schema that has been preinitialized with HTML
elements, attributes, and character entity declarations.  All the declarations
normally provided with HTML 4.01 are given, plus some that are IE-specific
and NS4-specific.  Attribute declarations of type CDATA with no default
value are not included.
*/

package org.ccil.cowan.tagsoup;
public class HTMLSchema extends Schema implements HTMLModels {

	/**
	Returns a newly constructed HTMLSchema object independent of
	any existing ones.
	*/

	public HTMLSchema() {
		// Start of Schema calls
		@@SCHEMA_CALLS@@
		// End of Schema calls
		}


	}
