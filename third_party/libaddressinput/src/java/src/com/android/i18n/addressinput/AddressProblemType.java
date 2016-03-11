/*
 * Copyright (C) 2010 Google Inc.
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

package com.android.i18n.addressinput;

/**
 * Enumerates problems that default address verification can report.
 */
public enum AddressProblemType {

    /**
     * The field is not null and not whitespace, and the field should not be used for this country.
     *
     * <p>For example, in the U.S. the SORTING_CODE field is unused, so its presence is an error.
     */
    USING_UNUSED_FIELD,

    /**
     * The field is null or whitespace, and the field is required.
     *
     * <p>For example, in the U.S. ADMIN_AREA is a required field.
     */
    MISSING_REQUIRED_FIELD,

    /**
     * A list of values for the field is defined and the value does not occur in the list. Applies
     * to hierarchical elements like REGION, ADMIN_AREA, LOCALITY, and DEPENDENT_LOCALITY.
     *
     * <p>For example, in the U.S. the only valid values for ADMIN_AREA are the two-letter state
     * codes.
     */
    UNKNOWN_VALUE,

    /**
     * A format for the field is defined and the value does not match. This is used to match
     * POSTAL_CODE against the the format pattern generally.
     *
     * <p>For example, in the U.S. postal codes are five digits with an optional hyphen followed by
     * four digits.
     */
    UNRECOGNIZED_FORMAT,

    /**
     * A pattern for the field is defined and the value does not match. This is used to match
     * POSTAL_CODE against a regular expression.
     *
     * <p>For example, in the U.S. postal codes in the state of California start with '9'.
     */
    MISMATCHING_VALUE;

    /**
     * Returns a unique string identifying this problem (for use in a message catalog).
     */
    public String keyname() {
        return name().toLowerCase();
    }
}
