/*
 * If not stated otherwise in this file or this component's license file the
 * following copyright and licenses apply:
 *
 * Copyright 2015 RDK Management
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
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <glib.h>
#include <string.h>
#include <vector>
#include "ctrlm_rf4ce_network.h"

using namespace std;

/*
  The purpose of this file is to convert remote battery voltage reported to the stb to a percent.
  This percent value is shown to the user and also logged by the stb at various times.
  The remote sends the stb both loaded and unloaded voltage reading.  In the past, we relied
  on the loaded voltage value to be used in the table.  We are now going to be using 
  unloaded voltage because the XR15v1, XR15v2, and XR16 put an artificial load for the loaded voltage.
  This would only cause the battery to drain faster.  So to keep everything the same across remotes,
  the unloaded voltage will be used on all the arrays/tables.

  In the past, there were only 2 tables one for the old XR11 and all the other remotes.
  Now we will have a table for each remote type to hopefully more accurately show the battery capacity (percent).
  The old/original/regular XR11s had a problem where they would only be operational from +3v to around 2.4v
  using the loaded battery voltage.  The new XR11s have a normal range of +3v to around 2.14v.
  Almost all the XR11s in the Comcast footprint are the old XR11s with the problem.  The new XR11s
  are sometimes referred as the XR11 HW Fixed.

  We have collected battery voltage details over the years, so now we are going to work backwards
  from that data to create more accurate tables.  Unfortunately the reported data only is reported 
  into 4 buckets: 100-75, 75-50, 50-25, 25-0.  Sal pulled the data and found the following results: 6/2021

                     100-75   75-50    50-25    25-0
      XR16:          136,     28.4,    24.4,    20.3
      XR15v2:        173.5,   65,      35.1,    29.2  
      XR15v1:        153,     60.5,    51.6,    37.6
      XR11 (old):    115.4,   70.8,    56,      44.4    

  The number represents days in that bucket.  Notice that the 100-75 is very long compared to the 25-0 bucket days.
  We want to try and make all the buckets the same in duration, so that when 75% is shown to the user
  then they have confidence that they still have 75% battery capacity.

  So, we need to massage the tables to shift days out of the 100-75 to the other buckets.
  How do we do that?
  By changing the percent value returned at different voltages.  To lower the days in the 100-75 bucket
  then we need to start returning 75 or less at a higher voltage.  As the voltage drops in the remote
  then it will switch to the 75-50 bucket sooner, so less days are spent in the 100-75 bucket.

  Voltage number is 8 bit binary representation
      2 high bits are the volt steps
      6 lower bits are right of decimal point in steps of x/64
     
     137 = 0x89, 10 00 1001
                  2 most sig bits = 10 = 2v
                  9/64 = .14 v
                  2.14 v

*/


#define V2P_INDEX_XR2_LINEAR_PERCENT_ZERO (128)
#define V2P_INDEX_XR2_LINEAR_PERCENT_100  (191)

const char v2p_table_xr2_linear[64] = {
     1, // Index 128 Voltage 2.01
     2, // Index 129 Voltage 2.02
     4, // Index 130 Voltage 2.04
     5, // Index 131 Voltage 2.05
     7, // Index 132 Voltage 2.07
     9, // Index 133 Voltage 2.09
    10, // Index 134 Voltage 2.10
    12, // Index 135 Voltage 2.12
    13, // Index 136 Voltage 2.13
    15, // Index 137 Voltage 2.15
    16, // Index 138 Voltage 2.16
    18, // Index 139 Voltage 2.18
    20, // Index 140 Voltage 2.20
    21, // Index 141 Voltage 2.21
    23, // Index 142 Voltage 2.23
    24, // Index 143 Voltage 2.24
    26, // Index 144 Voltage 2.26
    27, // Index 145 Voltage 2.27
    29, // Index 146 Voltage 2.29
    31, // Index 147 Voltage 2.31
    32, // Index 148 Voltage 2.32
    34, // Index 149 Voltage 2.34
    35, // Index 150 Voltage 2.35
    37, // Index 151 Voltage 2.37
    38, // Index 152 Voltage 2.38
    40, // Index 153 Voltage 2.40
    42, // Index 154 Voltage 2.42
    43, // Index 155 Voltage 2.43
    45, // Index 156 Voltage 2.45
    46, // Index 157 Voltage 2.46
    48, // Index 158 Voltage 2.48
    49, // Index 159 Voltage 2.49
    51, // Index 160 Voltage 2.51
    53, // Index 161 Voltage 2.53
    54, // Index 162 Voltage 2.54
    56, // Index 163 Voltage 2.56
    57, // Index 164 Voltage 2.57
    59, // Index 165 Voltage 2.59
    60, // Index 166 Voltage 2.60
    62, // Index 167 Voltage 2.62
    64, // Index 168 Voltage 2.64
    65, // Index 169 Voltage 2.65
    67, // Index 170 Voltage 2.67
    68, // Index 171 Voltage 2.68
    70, // Index 172 Voltage 2.70
    71, // Index 173 Voltage 2.71
    73, // Index 174 Voltage 2.73
    75, // Index 175 Voltage 2.75
    76, // Index 176 Voltage 2.76
    78, // Index 177 Voltage 2.78
    79, // Index 178 Voltage 2.79
    81, // Index 179 Voltage 2.81
    82, // Index 180 Voltage 2.82
    84, // Index 181 Voltage 2.84
    85, // Index 182 Voltage 2.85
    87, // Index 183 Voltage 2.87
    89, // Index 184 Voltage 2.89
    90, // Index 185 Voltage 2.90
    92, // Index 186 Voltage 2.92
    93, // Index 187 Voltage 2.93
    95, // Index 188 Voltage 2.95
    96, // Index 189 Voltage 2.96
    98, // Index 190 Voltage 2.98
   100, // Index 191 Voltage 3.00
};

#define V2P_INDEX_XR11_REG_PERCENT_ZERO (147)
#define V2P_INDEX_XR11_REG_PERCENT_100  (191)

/* 
XR11v1
Old XR11s without the fix
these make up most of the XR11s in the Comcast footprint
For XR11v2, 100% battery is 2.9516v and above.
Expected battery life is 641 days.
2.7916v at 160 days
2.6316v at 320 days
2.4716v at 480 days

*/

const char v2p_table_xr11_reg[45] = {
    2, // Index 147 Voltage 2.31
    4, // Index 148 Voltage 2.32
    6, // Index 149 Voltage 2.34
    8, // Index 150 Voltage 2.35
    10, // Index 151 Voltage 2.37
    12, // Index 152 Voltage 2.38
    14, // Index 153 Voltage 2.40
    16, // Index 154 Voltage 2.42
    18, // Index 155 Voltage 2.43
    20, // Index 156 Voltage 2.45
    22, // Index 157 Voltage 2.46
    24, // Index 158 Voltage 2.48   24-0 Bucket
    26, // Index 159 Voltage 2.49
    28, // Index 160 Voltage 2.51
    31, // Index 161 Voltage 2.53
    35, // Index 162 Voltage 2.54
    39, // Index 163 Voltage 2.56
    41, // Index 164 Voltage 2.57
    43, // Index 165 Voltage 2.59
    45, // Index 166 Voltage 2.60
    49, // Index 167 Voltage 2.62   49-25 Bucket
    51, // Index 168 Voltage 2.64
    53, // Index 169 Voltage 2.65
    56, // Index 170 Voltage 2.67
    58, // Index 171 Voltage 2.68
    60, // Index 172 Voltage 2.70
    62, // Index 173 Voltage 2.71
    64, // Index 174 Voltage 2.73
    67, // Index 175 Voltage 2.75
    69, // Index 176 Voltage 2.76
    71, // Index 177 Voltage 2.78
    74, // Index 178 Voltage 2.79   74-50 Bucket
    76, // Index 179 Voltage 2.81
    77, // Index 180 Voltage 2.82
    78, // Index 181 Voltage 2.84
    80, // Index 182 Voltage 2.85
    81, // Index 183 Voltage 2.87
    83, // Index 184 Voltage 2.89
    85, // Index 185 Voltage 2.90
    88, // Index 186 Voltage 2.92
    90, // Index 187 Voltage 2.93
    92, // Index 188 Voltage 2.95
    94, // Index 189 Voltage 2.96
    98, // Index 190 Voltage 2.98
   100, // Index 191 Voltage 3.00   100-75 Bucket
};
#define V2P_INDEX_XR11_FIX_PERCENT_ZERO (137)
#define V2P_INDEX_XR11_FIX_PERCENT_100  (191)

/*
XR11v2
These XR11s are mainly deployed to partners.
Since we did not have any data on this remote, I made
it the same as XR15v2.  It does have a different chip
then the XR15v2, but it should be close.
*/

const char v2p_table_xr11_fix[55] = {
     1, // Index 137 Voltage 2.15
     2, // Index 138 Voltage 2.16
     3, // Index 139 Voltage 2.18
     4, // Index 140 Voltage 2.20
     6, // Index 141 Voltage 2.21
     8, // Index 142 Voltage 2.23
    9, // Index 143 Voltage 2.24
    10, // Index 144 Voltage 2.26
    12, // Index 145 Voltage 2.27
    14, // Index 146 Voltage 2.29
    16, // Index 147 Voltage 2.31 
    18, // Index 148 Voltage 2.32
    20, // Index 149 Voltage 2.34
    22, // Index 150 Voltage 2.35
    24, // Index 151 Voltage 2.37   24-0 Bucket
    26, // Index 152 Voltage 2.38
    27, // Index 153 Voltage 2.40
    28, // Index 154 Voltage 2.42
    29, // Index 155 Voltage 2.43
    31, // Index 156 Voltage 2.45 
    33, // Index 157 Voltage 2.46
    35, // Index 158 Voltage 2.48
    37, // Index 159 Voltage 2.49
    39, // Index 160 Voltage 2.51
    41, // Index 161 Voltage 2.53
    43, // Index 162 Voltage 2.54
    45, // Index 163 Voltage 2.56
    47, // Index 164 Voltage 2.57
    49, // Index 165 Voltage 2.59   49-25 Bucket
    51, // Index 166 Voltage 2.60, 
    52, // Index 167 Voltage 2.62
    54, // Index 168 Voltage 2.64
    56, // Index 169 Voltage 2.65
    58, // Index 170 Voltage 2.67
    60, // Index 171 Voltage 2.68
    62, // Index 172 Voltage 2.70
    64, // Index 173 Voltage 2.71
    66, // Index 174 Voltage 2.73
    68, // Index 175 Voltage 2.75
    70, // Index 176 Voltage 2.76
    72, // Index 177 Voltage 2.78
    74, // Index 178 Voltage 2.79   74-50 Bucket
    76, // Index 179 Voltage 2.81
    78, // Index 180 Voltage 2.82
    80, // Index 181 Voltage 2.84
    82, // Index 182 Voltage 2.85
    84, // Index 183 Voltage 2.87
    86, // Index 184 Voltage 2.89
    88, // Index 185 Voltage 2.90
    90, // Index 186 Voltage 2.92
    92, // Index 187 Voltage 2.93
    94, // Index 188 Voltage 2.95
    96, // Index 189 Voltage 2.96
    98, // Index 190 Voltage 2.98
   100, // Index 191 Voltage 3.00 100-75 Bucket
};



#define V2P_INDEX_XR15V1_PERCENT_ZERO (137)
#define V2P_INDEX_XR15V1_PERCENT_100  (191)
/*
Eric/Mohita data and Sal's query 6.30.2021
For XR15v1, 100% battery is 2.9802v and above.
Expected battery life is 692 days.
2.7726v at 173 days
2.565v at 346 days
2.3574v at 519 days
*/

const char v2p_table_xr15v1[55] = {
     1, // Index 137 Voltage 2.15
     2, // Index 138 Voltage 2.16
     3, // Index 139 Voltage 2.18
     4, // Index 140 Voltage 2.20
     6, // Index 141 Voltage 2.21
     8, // Index 142 Voltage 2.23
    10, // Index 143 Voltage 2.24
    12, // Index 144 Voltage 2.26
    14, // Index 145 Voltage 2.27
    16, // Index 146 Voltage 2.29
    18, // Index 147 Voltage 2.31
    20, // Index 148 Voltage 2.32
    22, // Index 149 Voltage 2.34
    24, // Index 150 Voltage 2.35   24-0 bucket
    26, // Index 151 Voltage 2.37
    27, // Index 152 Voltage 2.38
    29, // Index 153 Voltage 2.40
    31, // Index 154 Voltage 2.42
    33, // Index 155 Voltage 2.43
    35, // Index 156 Voltage 2.45
    37, // Index 157 Voltage 2.46
    39, // Index 158 Voltage 2.48
    41, // Index 159 Voltage 2.49
    43, // Index 160 Voltage 2.51
    45, // Index 161 Voltage 2.53
    47, // Index 162 Voltage 2.54
    49, // Index 163 Voltage 2.56   49-25 bucket   
    51, // Index 164 Voltage 2.57
    52, // Index 165 Voltage 2.59
    54, // Index 166 Voltage 2.60
    56, // Index 167 Voltage 2.62
    58, // Index 168 Voltage 2.64
    60, // Index 169 Voltage 2.65
    62, // Index 170 Voltage 2.67
    64, // Index 171 Voltage 2.68
    66, // Index 172 Voltage 2.70
    68, // Index 173 Voltage 2.71
    70, // Index 174 Voltage 2.73
    72, // Index 175 Voltage 2.75
    74, // Index 176 Voltage 2.76   74-50 bucket
    75, // Index 177 Voltage 2.78
    76, // Index 178 Voltage 2.79
    77, // Index 179 Voltage 2.81
    78, // Index 180 Voltage 2.82
    80, // Index 181 Voltage 2.84
    82, // Index 182 Voltage 2.85
    84, // Index 183 Voltage 2.87
    86, // Index 184 Voltage 2.89
    88, // Index 185 Voltage 2.90
    90, // Index 186 Voltage 2.92
    92, // Index 187 Voltage 2.93
    94, // Index 188 Voltage 2.95
    96, // Index 189 Voltage 2.96
    98, // Index 190 Voltage 2.98
   100, // Index 191 Voltage 3.00   100-75 bucket
};


#define V2P_INDEX_XR15V2_PERCENT_ZERO (137)
#define V2P_INDEX_XR15V2_PERCENT_100  (191)

/*
Eric/Mohita data and Sal's query 6.30.2021
For XR15v2, 100% battery is 3.0058v and above.
Expected battery life is 658 days.
2.7926v at 164 days
2.5794v at 328 days
2.3662v at 492 days
*/

const char v2p_table_xr15v2[55] = {
     1, // Index 137 Voltage 2.15
     2, // Index 138 Voltage 2.16
     3, // Index 139 Voltage 2.18
     4, // Index 140 Voltage 2.20
     6, // Index 141 Voltage 2.21
     8, // Index 142 Voltage 2.23
    9, // Index 143 Voltage 2.24
    10, // Index 144 Voltage 2.26
    12, // Index 145 Voltage 2.27
    14, // Index 146 Voltage 2.29
    16, // Index 147 Voltage 2.31 
    18, // Index 148 Voltage 2.32
    20, // Index 149 Voltage 2.34
    22, // Index 150 Voltage 2.35
    24, // Index 151 Voltage 2.37   24-0 Bucket
    26, // Index 152 Voltage 2.38
    27, // Index 153 Voltage 2.40
    28, // Index 154 Voltage 2.42
    29, // Index 155 Voltage 2.43
    31, // Index 156 Voltage 2.45 
    33, // Index 157 Voltage 2.46
    35, // Index 158 Voltage 2.48
    37, // Index 159 Voltage 2.49
    39, // Index 160 Voltage 2.51
    41, // Index 161 Voltage 2.53
    43, // Index 162 Voltage 2.54
    45, // Index 163 Voltage 2.56
    47, // Index 164 Voltage 2.57
    49, // Index 165 Voltage 2.59   59-25 Bucket
    51, // Index 166 Voltage 2.60, 
    52, // Index 167 Voltage 2.62
    54, // Index 168 Voltage 2.64
    56, // Index 169 Voltage 2.65
    58, // Index 170 Voltage 2.67
    60, // Index 171 Voltage 2.68
    62, // Index 172 Voltage 2.70
    64, // Index 173 Voltage 2.71
    66, // Index 174 Voltage 2.73
    68, // Index 175 Voltage 2.75
    70, // Index 176 Voltage 2.76
    72, // Index 177 Voltage 2.78
    74, // Index 178 Voltage 2.79   74-50 Bucket
    76, // Index 179 Voltage 2.81
    78, // Index 180 Voltage 2.82
    80, // Index 181 Voltage 2.84
    82, // Index 182 Voltage 2.85
    84, // Index 183 Voltage 2.87
    86, // Index 184 Voltage 2.89
    88, // Index 185 Voltage 2.90
    90, // Index 186 Voltage 2.92
    92, // Index 187 Voltage 2.93
    94, // Index 188 Voltage 2.95
    96, // Index 189 Voltage 2.96
    98, // Index 190 Voltage 2.98
   100, // Index 191 Voltage 3.00   100-75 Bucket
};

#define V2P_INDEX_XR16_PERCENT_ZERO (137)
#define V2P_INDEX_XR16_PERCENT_100  (191)

/*
Eric/Mohita data and Sal's query 6.30.2021
For XR16, 100% battery is 2.9255v and above.
Expected battery life is 287 days.
2.7338v at 71 days
2.5421v at 142 days
2.3504v at 213 days
*/

const char v2p_table_xr16[55] = {
     1, // Index 137 Voltage 2.15
     2, // Index 138 Voltage 2.16
     3, // Index 139 Voltage 2.18
     4, // Index 140 Voltage 2.20
     6, // Index 141 Voltage 2.21
     8, // Index 142 Voltage 2.23
    10, // Index 143 Voltage 2.24
    12, // Index 144 Voltage 2.26
    15, // Index 145 Voltage 2.27
    16, // Index 146 Voltage 2.29
    18, // Index 147 Voltage 2.31
    20, // Index 148 Voltage 2.32
    22, // Index 149 Voltage 2.34
    24, // Index 150 Voltage 2.35   24-0 Bucket
    27, // Index 151 Voltage 2.37
    29, // Index 152 Voltage 2.38
    31, // Index 153 Voltage 2.40
    33, // Index 154 Voltage 2.42
    35, // Index 155 Voltage 2.43
    37, // Index 156 Voltage 2.45
    39, // Index 157 Voltage 2.46
    41, // Index 158 Voltage 2.48
    43, // Index 159 Voltage 2.49
    45, // Index 160 Voltage 2.51
    47, // Index 161 Voltage 2.53
    49, // Index 162 Voltage 2.54   49-25 Bucket
    52, // Index 163 Voltage 2.56
    54, // Index 164 Voltage 2.57
    56, // Index 165 Voltage 2.59
    58, // Index 166 Voltage 2.60
    60, // Index 167 Voltage 2.62
    62, // Index 168 Voltage 2.64
    64, // Index 169 Voltage 2.65
    66, // Index 170 Voltage 2.67
    68, // Index 171 Voltage 2.68
    70, // Index 172 Voltage 2.70
    72, // Index 173 Voltage 2.71
    74, // Index 174 Voltage 2.73   74-50 Bucket
    75, // Index 175 Voltage 2.75
    75, // Index 176 Voltage 2.76
    76, // Index 177 Voltage 2.78
    77, // Index 178 Voltage 2.79
    78, // Index 179 Voltage 2.81
    79, // Index 180 Voltage 2.82
    80, // Index 181 Voltage 2.84
    82, // Index 182 Voltage 2.85
    84, // Index 183 Voltage 2.87
    86, // Index 184 Voltage 2.89
    88, // Index 185 Voltage 2.90
    90, // Index 186 Voltage 2.92
    92, // Index 187 Voltage 2.93
    94, // Index 188 Voltage 2.95
    96, // Index 189 Voltage 2.96
    98, // Index 190 Voltage 2.98
   100, // Index 191 Voltage 3.00   100-75 Bucket
};

#define V2P_INDEX_XRA_PERCENT_ZERO (137)
#define V2P_INDEX_XRA_PERCENT_100  (191)

/*
Since we did not have any data on this remote, I made
it the same as XR15v2.  It does have a different chip
then the XR15v2, but it should be close
*/

const char v2p_table_xra[55] = {
     1, // Index 137 Voltage 2.15
     2, // Index 138 Voltage 2.16
     3, // Index 139 Voltage 2.18
     4, // Index 140 Voltage 2.20
     6, // Index 141 Voltage 2.21
     8, // Index 142 Voltage 2.23
    9, // Index 143 Voltage 2.24
    10, // Index 144 Voltage 2.26
    12, // Index 145 Voltage 2.27
    14, // Index 146 Voltage 2.29
    16, // Index 147 Voltage 2.31 
    18, // Index 148 Voltage 2.32
    20, // Index 149 Voltage 2.34
    22, // Index 150 Voltage 2.35
    24, // Index 151 Voltage 2.37   24-0 Bucket
    26, // Index 152 Voltage 2.38
    27, // Index 153 Voltage 2.40
    28, // Index 154 Voltage 2.42
    29, // Index 155 Voltage 2.43
    31, // Index 156 Voltage 2.45 
    33, // Index 157 Voltage 2.46
    35, // Index 158 Voltage 2.48
    37, // Index 159 Voltage 2.49
    39, // Index 160 Voltage 2.51
    41, // Index 161 Voltage 2.53
    43, // Index 162 Voltage 2.54
    45, // Index 163 Voltage 2.56
    47, // Index 164 Voltage 2.57
    49, // Index 165 Voltage 2.59   49-25 Bucket
    51, // Index 166 Voltage 2.60, 
    52, // Index 167 Voltage 2.62
    54, // Index 168 Voltage 2.64
    56, // Index 169 Voltage 2.65
    58, // Index 170 Voltage 2.67
    60, // Index 171 Voltage 2.68
    62, // Index 172 Voltage 2.70
    64, // Index 173 Voltage 2.71
    66, // Index 174 Voltage 2.73
    68, // Index 175 Voltage 2.75
    70, // Index 176 Voltage 2.76
    72, // Index 177 Voltage 2.78
    74, // Index 178 Voltage 2.79   74-50 Bucket
    76, // Index 179 Voltage 2.81
    78, // Index 180 Voltage 2.82
    80, // Index 181 Voltage 2.84
    82, // Index 182 Voltage 2.85
    84, // Index 183 Voltage 2.87
    86, // Index 184 Voltage 2.89
    88, // Index 185 Voltage 2.90
    90, // Index 186 Voltage 2.92
    92, // Index 187 Voltage 2.93
    94, // Index 188 Voltage 2.95
    96, // Index 189 Voltage 2.96
    98, // Index 190 Voltage 2.98
   100, // Index 191 Voltage 3.00 100-75 Bucket
};


uint8_t battery_level_percent(int type, ctrlm_hw_version_t hw_version, unsigned char voltage_unloaded) {
   switch(type) {
      case RF4CE_CONTROLLER_TYPE_XR11: {
         ctrlm_hw_version_t xr11_version_min(2, 2, 2, 0);
         if(hw_version >= xr11_version_min) { // if hardware version is >= 2.2.2.0
            if(voltage_unloaded > V2P_INDEX_XR11_FIX_PERCENT_100) {
               return(100);
            } else if(voltage_unloaded < V2P_INDEX_XR11_FIX_PERCENT_ZERO) {
               return(0);
            } else {
               return(v2p_table_xr11_fix[voltage_unloaded - V2P_INDEX_XR11_FIX_PERCENT_ZERO]);
            }
         } else {
            if(voltage_unloaded > V2P_INDEX_XR11_REG_PERCENT_100) {
               return(100);
            } else if(voltage_unloaded < V2P_INDEX_XR11_REG_PERCENT_ZERO) {
               return(0);
            } else {
               return(v2p_table_xr11_reg[voltage_unloaded - V2P_INDEX_XR11_REG_PERCENT_ZERO]);
            }
         }
         break;
      }
      case RF4CE_CONTROLLER_TYPE_XR15:
      {
         if(voltage_unloaded > V2P_INDEX_XR15V1_PERCENT_100) {
            return(100);
         } else if(voltage_unloaded < V2P_INDEX_XR15V1_PERCENT_ZERO) {
            return(0);
         } else {
            return(v2p_table_xr15v1[voltage_unloaded - V2P_INDEX_XR15V1_PERCENT_ZERO]);
         }
         break;
      }
      case RF4CE_CONTROLLER_TYPE_XR15V2: 
      {
         if(voltage_unloaded > V2P_INDEX_XR15V2_PERCENT_100) {
            return(100);
         } else if(voltage_unloaded < V2P_INDEX_XR15V2_PERCENT_ZERO) {
            return(0);
         } else {
            return(v2p_table_xr15v2[voltage_unloaded - V2P_INDEX_XR15V2_PERCENT_ZERO]);
         }
         break;
      }
      case RF4CE_CONTROLLER_TYPE_XR16:
      {
         if(voltage_unloaded > V2P_INDEX_XR16_PERCENT_100) {
            return(100);
         } else if(voltage_unloaded < V2P_INDEX_XR16_PERCENT_ZERO) {
            return(0);
         } else {
            return(v2p_table_xr16[voltage_unloaded - V2P_INDEX_XR16_PERCENT_ZERO]);
         }
         break;
      }
      case RF4CE_CONTROLLER_TYPE_XRA: 
      {
         if(voltage_unloaded > V2P_INDEX_XRA_PERCENT_100) {
            return(100);
         } else if(voltage_unloaded < V2P_INDEX_XRA_PERCENT_ZERO) {
            return(0);
         } else {
            return(v2p_table_xra[voltage_unloaded - V2P_INDEX_XRA_PERCENT_ZERO]);
         }
         break;
      }
      default: {
         if(voltage_unloaded > V2P_INDEX_XR2_LINEAR_PERCENT_100) {
            return(100);
         } else if(voltage_unloaded < V2P_INDEX_XR2_LINEAR_PERCENT_ZERO) {
            return(0);
         } else {
            return(v2p_table_xr2_linear[voltage_unloaded - V2P_INDEX_XR2_LINEAR_PERCENT_ZERO]);
         }
         break;
      }
   }
}