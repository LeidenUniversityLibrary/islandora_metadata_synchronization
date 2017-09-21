<?php

/**
 * @file
 * extract a identifier from a specific datastream of a specific object
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
 * Find an identifier within an object's DSID, optionally with an XPath.
 */
function islandora_metadata_synchronization_get_identifier(AbstractObject $object, $dsid, $xpath = NULL) {
  $syncid = NULL;
  if (isset($object[$dsid])) {
    $content = $object[$dsid]->content;
    if (isset($xpath) && strlen($xpath) > 0) {
      $domdoc = new DOMDocument();
      if ($domdoc->loadXML($content)) {
        $domxpath = islandora_metadata_synchronization_new_xpath($domdoc);
        $domnodelist = $domxpath->query($xpath);
        if ($domnodelist->length > 0) {
          foreach ($domnodelist as $domnode) {
            $text = $domnode->textContent;
            if (isset($text) && strlen($text) > 0) {
              $syncid = $text;
              break;
            }
          }
        }
      }
      else {
        drupal_set_message(t('Datastream @dsid contains invalid XML data', array('@dsid' => $dsid)), 'warning');
      }
    }
    else {
      $syncid = $content;
    }
  }
  return $syncid;
}

/**
 * Helper function: create a new DOMXPath with all namespaces.
 */
function islandora_metadata_synchronization_new_xpath($domdoc) {
  $domxpath = new DOMXPath($domdoc);
  $domxpath->registerNamespace('mods', 'http://www.loc.gov/mods/v3');
  $domxpath->registerNamespace('dc', 'http://purl.org/dc/elements/1.1/');
  $domxpath->registerNamespace('marc', 'http://www.loc.gov/MARC21/slim');
  $domxpath->registerNamespace('oai', 'http://www.openarchives.org/OAI/2.0/');

  return $domxpath;
}
