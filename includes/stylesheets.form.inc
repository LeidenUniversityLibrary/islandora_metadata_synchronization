<?php
/**
 * @file
 * Stylesheet files management.
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
 * Renders a file management form.
 *
 * @param array $form
 *   An array representing a Drupal form.
 * @param array $form_state
 *   An array containing the Drupal form state.
 *
 * @return array
 *   An array representing the file management form.
 */
function islandora_metadata_synchronization_manage_stylesheets_form($form, &$form_state) {
  // Grab all the user uploaded files.
  $xsl_files_struct = array();
  $upload_path = 'public://islandora_metadata_synchronization_xsls';
  $xsl_current_files = file_scan_directory($upload_path, '/.*\.xslt?$/');

  foreach ($xsl_current_files as $up_file) {
    $xsl_files_struct[$up_file->uri] = array(
      $up_file->filename,
      l(t('download'), file_create_url($up_file->uri)),
    );
  }
  ksort($xsl_files_struct);
  $form['islandora_md_sync_xsl_files'] = array(
    '#type' => 'fieldset',
    '#title' => t('Files'),
    '#collapsed' => FALSE,
    '#collapsible' => FALSE,
  );
  $form['islandora_md_sync_xsl_files']['table'] = array(
    '#type' => 'tableselect',
    '#header' => array(
      t('File name'),
      t('Operations'),
    ),
    '#options' => $xsl_files_struct,
    '#empty' => t('No uploaded files!'),
  );
  if (count($xsl_files_struct)) {
    $form['islandora_md_sync_xsl_files']['remove_selected'] = array(
      '#type' => 'submit',
      '#value' => t('Delete selected'),
    );
  }
  $form['islandora_md_sync_xsl_files']['islandora_md_sync_xsl_upload'] = array(
    '#type' => 'fieldset',
    '#title' => t('Upload'),
    '#collapsible' => FALSE,
    '#collapsed' => FALSE,
  );
  $form['islandora_md_sync_xsl_files']['islandora_md_sync_xsl_upload']['islandora_md_sync_xsl_upload_xsl'] = array(
    '#type' => 'file',
    '#title' => t('Upload an XSL or XSLT file to be used for transformations'),
    '#upload_location' => 'public://islandora_metadata_synchronization_xsls',
    '#upload_validators' => array(
      'file_validate_extensions' => array('xsl xslt'),
    ),
  );
  $form['islandora_md_sync_xsl_files']['islandora_md_sync_xsl_upload']['islandora_md_sync_xsl_upload_button'] = array(
    '#type' => 'submit',
    '#value' => t('Upload'),
    '#submit' => array('islandora_metadata_synchronizatio_stylesheet_upload'),
  );
  return $form;
}

/**
 * Validation for the islandora_metadata_synchronization_manage_stylesheets form.
 *
 * @param array $form
 *   An array representing a Drupal form.
 * @param array $form_state
 *   An array containing the Drupal form state.
 */
function islandora_metadata_synchronization_manage_stylesheets_form_validate($form, $form_state) {
  if (end($form_state['triggering_element']['#parents']) == 'remove_selected') {
    $selected = FALSE;
    foreach ($form_state['values']['table'] as $value) {
      if ($value !== 0) {
        $selected = TRUE;
        break;
      }
    }
    if (!$selected) {
      form_set_error('table', t('Must select at least one entry to delete!'));
    }
  }
}
/**
 * Submit handler for the islandora_metadata_synchronization_manage_stylesheets form.
 *
 * @param array $form
 *   An array representing a Drupal form.
 * @param array $form_state
 *   An array containing the Drupal form state.
 */
function islandora_metadata_synchronization_manage_stylesheets_form_submit($form, &$form_state) {
  $deleted_count = 0;
  $fid_or = db_or();
  foreach ($form_state['values']['table'] as $uri => $selected) {
    if ($selected !== 0) {
      $fid_or->condition('uri', $uri, '=');
    }
  }
  $fids = db_select('file_managed', 'f')
    ->fields('f', array('fid', 'uri'))
    ->condition($fid_or)
    ->execute()
    ->fetchAllAssoc('fid', PDO::FETCH_ASSOC);
  foreach ($fids as $fid) {
    file_delete(file_load($fid['fid']));
    $deleted_count++;
  }
  if ($deleted_count > 0) {
    drupal_set_message(format_plural($deleted_count,
      'Successfully deleted 1 file!',
      'Successfully deleted @count files!'
    ));
  }
}
/**
 * Submit handler for uploading a file.
 *
 * @param array $form
 *   An array representing a Drupal form.
 * @param array $form_state
 *   An array containing the Drupal form state.
 */
function islandora_metadata_synchronizatio_stylesheet_upload($form, $form_state) {
  $file = file_save_upload('islandora_md_sync_xsl_upload_xsl', array('file_validate_extensions' => array('xsl xslt')), 'public://islandora_metadata_synchronization_xsls');
  if ($file) {
    $file->status = FILE_STATUS_PERMANENT;
    file_save($file);
    drupal_set_message(t("The file @filename was uploaded successfully.",
      array(
        '@filename' => $file->filename,
      )
    ));
  }
}
