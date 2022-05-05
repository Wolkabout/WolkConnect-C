/*
 * Copyright 2022 WolkAbout Technology s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "types.h"
#include "utility/wolk_utils.h"

const char UNIT_NUMERIC[] = "NUMERIC";
const char UNIT_BOOLEAN[] = "BOOLEAN";
const char UNIT_PERCENT[] = "PERCENT";
const char UNIT_DECIBEL[] = "DECIBEL";
const char UNIT_LOCATION[] = "LOCATION";
const char UNIT_TEXT[] = "TEXT";
const char UNIT_METRES_PER_SQUARE_SECOND[] = "METRES_PER_SQUARE_SECOND";
const char UNIT_G[] = "G";
const char UNIT_MOLE[] = "MOLE";
const char UNIT_ATOM[] = "ATOM";
const char UNIT_RADIAN[] = "RADIAN";
const char UNIT_REVOLUTION[] = "REVOLUTION";
const char UNIT_DEGREE_ANGLE[] = "DEGREE_ANGLE";
const char UNIT_MINUTE_ANGLE[] = "MINUTE_ANGLE";
const char UNIT_SECOND_ANGLE[] = "SECOND_ANGLE";
const char UNIT_CENTIRADIAN[] = "CENTIRADIAN";
const char UNIT_GRADE[] = "GRADE";
const char UNIT_SQUARE_METRE[] = "SQUARE_METRE";
const char UNIT_ARE[] = "ARE";
const char UNIT_HECTARE[] = "HECTARE";
const char UNIT_KATAL[] = "KATAL";
const char UNIT_BIT[] = "BIT";
const char UNIT_BYTE[] = "BYTE";
const char UNIT_SECOND[] = "SECOND";
const char UNIT_MINUTE[] = "MINUTE";
const char UNIT_HOUR[] = "HOUR";
const char UNIT_DAY[] = "DAY";
const char UNIT_WEEK[] = "WEEK";
const char UNIT_YEAR[] = "YEAR";
const char UNIT_MONTH[] = "MONTH";
const char UNIT_DAY_SIDEREAL[] = "DAY_SIDEREAL";
const char UNIT_YEAR_SIDEREAL[] = "YEAR_SIDEREAL";
const char UNIT_YEAR_CALENDAR[] = "YEAR_CALENDAR";
const char UNIT_POISE[] = "POISE";
const char UNIT_FARAD[] = "FARAD";
const char UNIT_COULOMB[] = "COULOMB";
const char UNIT_E[] = "E";
const char UNIT_FARADAY[] = "FARADAY";
const char UNIT_FRANKLIN[] = "FRANKLIN";
const char UNIT_SIEMENS[] = "SIEMENS";
const char UNIT_AMPERE[] = "AMPERE";
const char UNIT_GILBERT[] = "GILBERT";
const char UNIT_HENRY[] = "HENRY";
const char UNIT_VOLT[] = "VOLT";
const char UNIT_CENTIVOLT[] = "CENTIVOLT";
const char UNIT_MILLIVOLT[] = "MILLIVOLT";
const char UNIT_OHM[] = "OHM";
const char UNIT_JOULE[] = "JOULE";
const char UNIT_ERG[] = "ERG";
const char UNIT_ELECTRON_VOLT[] = "ELECTRON_VOLT";
const char UNIT_NEWTON[] = "NEWTON";
const char UNIT_DYNE[] = "DYNE";
const char UNIT_KILOGRAM_FORCE[] = "KILOGRAM_FORCE";
const char UNIT_POUND_FORCE[] = "POUND_FORCE";
const char UNIT_HERTZ[] = "HERTZ";
const char UNIT_MEGAHERTZ[] = "MEGAHERTZ";
const char UNIT_GIGAHERTZ[] = "GIGAHERTZ";
const char UNIT_LUX[] = "LUX";
const char UNIT_LAMBERT[] = "LAMBERT";
const char UNIT_STOKE[] = "STOKE";
const char UNIT_METRE[] = "METRE";
const char UNIT_KILOMETRE[] = "KILOMETRE";
const char UNIT_CENTIMETRE[] = "CENTIMETRE";
const char UNIT_MILLIMETRE[] = "MILLIMETRE";
const char UNIT_FOOT[] = "FOOT";
const char UNIT_FOOT_SURVEY_US[] = "FOOT_SURVEY_US";
const char UNIT_YARD[] = "YARD";
const char UNIT_INCH[] = "INCH";
const char UNIT_MILE[] = "MILE";
const char UNIT_NAUTICAL_MILE[] = "NAUTICAL_MILE";
const char UNIT_ANGSTROM[] = "ANGSTROM";
const char UNIT_ASTRONOMICAL_UNIT[] = "ASTRONOMICAL_UNIT";
const char UNIT_LIGHT_YEAR[] = "LIGHT_YEAR";
const char UNIT_PARSEC[] = "PARSEC";
const char UNIT_POINT[] = "POINT";
const char UNIT_PIXEL[] = "PIXEL";
const char UNIT_LUMEN[] = "LUMEN";
const char UNIT_CANDELA[] = "CANDELA";
const char UNIT_WEBER[] = "WEBER";
const char UNIT_MAXWELL[] = "MAXWELL";
const char UNIT_TESLA[] = "TESLA";
const char UNIT_GAUSS[] = "GAUSS";
const char UNIT_KILOGRAM[] = "KILOGRAM";
const char UNIT_GRAM[] = "GRAM";
const char UNIT_ATOMIC_MASS[] = "ATOMIC_MASS";
const char UNIT_ELECTRON_MASS[] = "ELECTRON_MASS";
const char UNIT_POUND[] = "POUND";
const char UNIT_OUNCE[] = "OUNCE";
const char UNIT_TON_US[] = "TON_US";
const char UNIT_TON_UK[] = "TON_UK";
const char UNIT_METRIC_TON[] = "METRIC_TON";
const char UNIT_WATT[] = "WATT";
const char UNIT_HORSEPOWER[] = "HORSEPOWER";
const char UNIT_PASCAL[] = "PASCAL";
const char UNIT_HECTOPASCAL[] = "HECTOPASCAL";
const char UNIT_ATMOSPHERE[] = "ATMOSPHERE";
const char UNIT_BAR[] = "BAR";
const char UNIT_MILLIBAR[] = "MILLIBAR";
const char UNIT_MILLIMETER_OF_MERCURY[] = "MILLIMETER_OF_MERCURY";
const char UNIT_INCH_OF_MERCURY[] = "INCH_OF_MERCURY";
const char UNIT_GRAY[] = "GRAY";
const char UNIT_RAD[] = "RAD";
const char UNIT_SIEVERT[] = "SIEVERT";
const char UNIT_REM[] = "REM";
const char UNIT_BECQUEREL[] = "BECQUEREL";
const char UNIT_CURIE[] = "CURIE";
const char UNIT_RUTHERFORD[] = "RUTHERFORD";
const char UNIT_ROENTGEN[] = "ROENTGEN";
const char UNIT_STERADIAN[] = "STERADIAN";
const char UNIT_SPHERE[] = "SPHERE";
const char UNIT_KELVIN[] = "KELVIN";
const char UNIT_CELSIUS[] = "CELSIUS";
const char UNIT_RANKINE[] = "RANKINE";
const char UNIT_FAHRENHEIT[] = "FAHRENHEIT";
const char UNIT_METRES_PER_SECOND[] = "METRES_PER_SECOND";
const char UNIT_MILES_PER_HOUR[] = "MILES_PER_HOUR";
const char UNIT_KILOMETRES_PER_HOUR[] = "KILOMETRES_PER_HOUR";
const char UNIT_KNOT[] = "KNOT";
const char UNIT_MACH[] = "MACH";
const char UNIT_C[] = "C";
const char UNIT_CUBIC_METRE[] = "CUBIC_METRE";
const char UNIT_LITRE[] = "LITRE";
const char UNIT_DECILITRE[] = "DECILITRE";
const char UNIT_MILLILITRE[] = "MILLILITRE";
const char UNIT_CUBIC_INCH[] = "CUBIC_INCH";
const char UNIT_GALLON_DRY_US[] = "GALLON_DRY_US";
const char UNIT_GALLON_UK[] = "GALLON_UK";
const char UNIT_OUNCE_LIQUID_UK[] = "OUNCE_LIQUID_UK";
const char UNIT_UNKNOWN[] = "UNKNOWN";

const char** FEED_TYPE[] = {"IN", "IN_OUT"};

const char DATA_TYPE_STRING[] = "STRING";
const char DATA_TYPE_NUMERIC[] = "NUMERIC";
const char DATA_TYPE_BOOLEAN[] = "BOOLEAN";
const char DATA_TYPE_HEXADECIMAL[] = "HEXADECIMAL";
const char DATA_TYPE_LOCATION[] = "LOCATION";
const char DATA_TYPE_ENUM[] = "ENUM";
const char DATA_TYPE_VECTOR[] = "VECTOR";

const char PARAMETER_CONNECTIVITY_TYPE[] = "CONNECTIVITY_TYPE";
const char PARAMETER_OUTBOUND_DATA_MODE[] = "OUTBOUND_DATA_MODE";
const char PARAMETER_OUTBOUND_DATA_RETENTION_TIME[] = "OUTBOUND_DATA_RETENTION_TIME";
const char PARAMETER_MAXIMUM_MESSAGE_SIZE[] = "MAXIMUM_MESSAGE_SIZE";
const char PARAMETER_FILE_TRANSFER_PLATFORM_ENABLED[] = "FILE_TRANSFER_PLATFORM_ENABLED";
const char PARAMETER_FILE_TRANSFER_URL_ENABLED[] = "FILE_TRANSFER_URL_ENABLED";
const char PARAMETER_FIRMWARE_UPDATE_ENABLED[] = "FIRMWARE_UPDATE_ENABLED";
const char PARAMETER_FIRMWARE_UPDATE_CHECK_TIME[] = "FIRMWARE_UPDATE_CHECK_TIME";
const char PARAMETER_FIRMWARE_UPDATE_REPOSITORY[] = "FIRMWARE_UPDATE_REPOSITORY";
const char PARAMETER_FIRMWARE_VERSION[] = "FIRMWARE_VERSION";
const char PARAMETER_GATEWAY[] = "GATEWAY";
const char PARAMETER_GATEWAY_PARENT[] = "GATEWAY_PARENT";
const char PARAMETER_EXTERNAL_ID[] = "EXTERNAL_ID";
const char PARAMETER_UNKNOWN[] = "UNKNOWN";

char* feed_type_to_string(feed_type_t feed_type)
{
    return FEED_TYPE[feed_type];
}

const char* file_management_status_as_str(file_management_status_t* status)
{
    /* Sanity check */
    WOLK_ASSERT(status);

    switch (status->state) {
    case FILE_MANAGEMENT_STATE_FILE_TRANSFER:
        return "FILE_TRANSFER";

    case FILE_MANAGEMENT_STATE_FILE_READY:
        return "FILE_READY";

    case FILE_MANAGEMENT_STATE_ERROR:
        return "ERROR";

    case FILE_MANAGEMENT_STATE_ABORTED:
        return "ABORTED";

    default:
        WOLK_ASSERT(false);
        return "";
    }
}
