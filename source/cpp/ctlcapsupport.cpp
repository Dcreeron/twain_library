/*
    This file is part of the Dynarithmic TWAIN Library (DTWAIN).
    Copyright (c) 2002-2019 Dynarithmic Software.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

    FOR ANY PART OF THE COVERED WORK IN WHICH THE COPYRIGHT IS OWNED BY
    DYNARITHMIC SOFTWARE. DYNARITHMIC SOFTWARE DISCLAIMS THE WARRANTY OF NON INFRINGEMENT
    OF THIRD PARTY RIGHTS.
 */
#include "ctltwmgr.h"
#include "enumeratorfuncs.h"
#include "errorcheck.h"
#include "ctltmpl5.h"
#ifdef _MSC_VER
#pragma warning (disable:4702)
#endif
using namespace std;
using namespace dynarithmic;

////////////////////////////////////////////////////////////////////////////////
DTWAIN_BOOL DLLENTRY_DEF DTWAIN_IsCapSupported(DTWAIN_SOURCE Source, LONG lCapability)
{
    LOG_FUNC_ENTRY_PARAMS((Source, lCapability))
    CTL_TwainDLLHandle *pHandle = static_cast<CTL_TwainDLLHandle *>(GetDTWAINHandle_Internal());
    CTL_ITwainSource *p = VerifySourceHandle(pHandle, Source);
    if (p)
    {
        // Check if the source is open
        if (!CTL_TwainAppMgr::IsSourceOpen(p))
            DTWAIN_Check_Error_Condition_0_Ex(pHandle, [] {return true; }, DTWAIN_ERR_SOURCE_NOT_OPEN, false, FUNC_MACRO);

        // Turn off error message box logging for this function
        DTWAINScopedLogControllerExclude sLogger(DTWAIN_LOG_ERRORMSGBOX);

        // Use the cap information from the arrays
        CTL_StringType strProdName = p->GetProductName();

        // Find where capability setting is
        int nWhere;
        FindFirstValue(strProdName, &pHandle->m_aSourceCapInfo, &nWhere);

        // Have we even done anything with capabilities for this Source.
        // This should not be -1 if the CAP_SUPPORTEDCAPS is supported when
        // the source was opened.

        // Check if cached (means that cap is supported and was already queried
        // for all information
        if (p->IsCapabilityCached((TW_UINT16)lCapability))
        {
            // Get the cap array values
            CTL_SourceCapInfo Info = pHandle->m_aSourceCapInfo[nWhere];
            CTL_CapInfoArrayPtr pArray = std::get<1>(Info);
            CTL_EnumCapability nCap = (CTL_EnumCapability)lCapability;
            if (pArray->find(nCap) != pArray->end())
                LOG_FUNC_EXIT_PARAMS(true)
            else
            {
                DTWAIN_CollectCapabilityInfo(p, (TW_UINT16)nCap, *pArray);
                LOG_FUNC_EXIT_PARAMS(true)
            }
        }

        // Check if in unsupported list
        if (p->IsCapInUnsupportedList(static_cast<TW_UINT16>(lCapability)))
        {
            DTWAIN_Check_Error_Condition_2_Ex(pHandle, [] {return true; }, DTWAIN_ERR_CAP_NO_SUPPORT, false, FUNC_MACRO);
        }

        // Check if slow cap retrieval is on
        if (!p->IsFastCapRetrieval())
        {
            // Get the cap using TWAIN.  Sources that do not support CAP_SUPPORTEDCAPS will most likely
            // be in this part of the code
            DTWAIN_BOOL bRet = CTL_TwainAppMgr::IsCapabilitySupported(p, (TW_UINT16)lCapability, CTL_GetTypeGET);
            if (bRet)
                // Cache the info now
                dynarithmic::DTWAIN_CacheCapabilityInfo(p, pHandle, static_cast<TW_UINT16>(lCapability));
            else
            {
                // Add this to list of caps not supported
                p->AddCapToUnsupportedList(static_cast<TW_UINT16>(lCapability));
                DTWAIN_Check_Error_Condition_2_Ex(pHandle, [] { return true; }, DTWAIN_ERR_CAP_NO_SUPPORT, false, FUNC_MACRO);
            }
            LOG_FUNC_EXIT_PARAMS(bRet)
        }
        else
            // Caps can be retrieved safely, since GetCapabilities() will use the CAP_SUPPORTEDCAPS
            // type, or the user has forced DTWAIN to use a brute force approach to enumerate
            // all of the capability values.
        {
            // Test if the capabilities have already been retrieved (but no info has
            // been gathered)
            if (!p->RetrievedAllCaps())
            {
                // Get the capabilities using TWAIN
                CTL_TwainCapArray rArray;
                CTL_TwainAppMgr::GetCapabilities(p, rArray);
                p->SetRetrievedAllCaps(true);
                p->SetCapSupportedList(rArray);
            }
            // Get the capabilities from the list in the Source
            CapList& pArray = p->GetCapSupportedList();

            bool bReturnVal = false;
            auto it = pArray.find(static_cast<TW_UINT16>(lCapability));
            if (it != pArray.end())
            {
                // Cap has been found, so finally get some stats and return
                bReturnVal = true;
                dynarithmic::DTWAIN_CacheCapabilityInfo(p, pHandle, static_cast<TW_UINT16>(lCapability));
                LOG_FUNC_EXIT_PARAMS(bReturnVal)
            }
            // cap not supported, so add to unsupported list
            p->AddCapToUnsupportedList(static_cast<TW_UINT16>(lCapability));
            DTWAIN_Check_Error_Condition_2_Ex(pHandle, []{ return true; },
            DTWAIN_ERR_CAP_NO_SUPPORT, false, FUNC_MACRO);
        }
    }
    LOG_FUNC_EXIT_PARAMS(false)
    CATCH_BLOCK(false)
}
