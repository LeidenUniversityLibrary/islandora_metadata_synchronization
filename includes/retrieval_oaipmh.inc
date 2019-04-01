<?php

/**
 * @file
 * Functions to retrieve metadata from a OAI-PMH compatible source.
 *
 *
 *  Copyright 2017 Leiden University Library
 *
 *  This file is part of islandora_metadata_synchronization.
 *
 *  islandora_metadata_synchronization is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


/**
 * Retrieve a record identified by syncid and mdprefix from a OAI-PMH source.
 */
function islandora_metadata_synchronization_retrieve_oaipmh_metadata($oaibaseurl, $syncid, $mdprefix) {
  if (!isset($oaibaseurl)) {
    drupal_set_message(t("Please provide a valid OAI-PMH url in settings"));
    return FALSE;
  }
  $oaigetrecurl = "$oaibaseurl?verb=GetRecord&metadataPrefix=$mdprefix&identifier=$syncid";

  $data = file_get_contents($oaigetrecurl);

  if ($data === FALSE) {
    return FALSE;
  }

  $domdoc = new DOMDocument();
  if ($domdoc->loadXML($data)) {
    $domxpath = islandora_metadata_synchronization_new_xpath($domdoc);
    $domnodelist = $domxpath->query('/oai:OAI-PMH/oai:error');
    if ($domnodelist->length == 1) {
      return FALSE;
    }
  }  

  return $data;
}

/**
 * Test if a OAI-PMH url is valid.
 */
function islandora_metadata_synchronization_test_oai_pmh_url($oaibaseurl) {
  if (isset($oaibaseurl)) {
    $oaiidurl = $oaibaseurl . "?verb=Identify";

    $data = @file_get_contents($oaiidurl);

    if (isset($data)) {
      $pattern = '#repositoryName>([^<]+)#s';
      $preg_result = preg_match($pattern, $data, $matches);

      if ($preg_result == 1) {
        return $matches[1];
      }
    }
  }
  return FALSE;
}

/**
 * Retrieve a set of identifiers for changed metadata from OAI-PMH.
 */
function islandora_metadata_synchronization_identifiers_for_changed_metadata($oaibaseurl, $params) {
  module_load_include('inc', 'islandora_metadata_synchronization', 'includes/objects_identifiers');
  if (isset($oaibaseurl)) {
    $oailiurl = $oaibaseurl . "?verb=ListIdentifiers";

    if (isset($params['resumptiontoken']) && strlen($params['resumptiontoken'])) {
      $oailiurl .= '&resumptionToken=' . $params['resumptiontoken'];
    }
    else {
      $oailiurl .= "&metadataPrefix=" . (isset($params['mdprefix']) ? $params['mdprefix'] : "oai_dc");
      if (isset($params['date']) && strlen($params['date'])) {
        $oailiurl .= "&from=" . $params['date'];
      }
      if (isset($params['set']) && strlen($params['set'])) {
        $oailiurl .= "&set=" . $params['set'];
      }
    }

    $data = @file_get_contents($oailiurl);

    if (isset($data)) {
      $result = array();

      $domdoc = new DOMDocument();
      if ($domdoc->loadXML($data)) {
        $domxpath = islandora_metadata_synchronization_new_xpath($domdoc);
        $domnodelist = $domxpath->query('/oai:OAI-PMH/oai:error');
        if ($domnodelist->length == 1) {
          $result["oaierror"] = $domnodelist->item(0)->textContent;
          $result["oaierrorcode"] = $domnodelist->item(0)->getAttribute('code');
        }
        $domnodelist = $domxpath->query('/oai:OAI-PMH/oai:responseDate');
        if ($domnodelist->length == 1) {
          $result["responsedate"] = $domnodelist->item(0)->textContent;
        }
        $domnodelist = $domxpath->query('/oai:OAI-PMH/oai:ListIdentifiers/oai:header');
        foreach ($domnodelist as $domnode) {
          $identifier = $domxpath->query('oai:identifier', $domnode);
          if (isset($identifier)) {
            $result["ids"][] = $identifier->item(0)->textContent;
          }
        }

        $domnodelist = $domxpath->query('/oai:OAI-PMH/oai:ListIdentifiers/oai:resumptionToken');
        if ($domnodelist->length == 1) {
          $next_resumption_token = $domnodelist->item(0)->textContent;
          if (strlen($next_resumption_token) > 0) {
            $result["resumptiontoken"] = $next_resumption_token;
          }
        }
      }
      return $result;
    }
  }
  return FALSE;
}
