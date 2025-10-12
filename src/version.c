/*
 * Copyright (C) 2025, jpn
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <ctoolbox/ctoolbox.h>


static const char versionstring[] = CTOOLBOX_VERSION_STRING;


struct CTOOLBOXVersion
ctoolbox_getversion(void)
{
	struct CTOOLBOXVersion v;

	v.major = CTOOLBOX_VERSION_MAJOR;
	v.minor = CTOOLBOX_VERSION_MINOR;
	v.patch = CTOOLBOX_VERSION_PATCH;

	v.versionstring = versionstring;
	v.builddate     = NULL;
	return v;
}
