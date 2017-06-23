<?php

/**
 * @file
 * Contains form functions for administrating of the metadata synchronization.
 */

/**
 * Calculate how many sources are defined in the admin form.
 */
function number_of_sources() {
  $nr_of_srcs = 1;
  for ($i = 1; $i <= 99; $i++) {
    $oaipmh_url = variable_get('islandora_metadata_synchronization_source' . $i . '_oaipmh_url');
    if (isset($oaipmh_url) && strlen($oaipmh_url) > 0) {
      $nr_of_srcs = $i;
    }
  }

  return $nr_of_srcs;
}

/**
 * Implements hook_form().
 */
function islandora_metadata_synchronization_start_sync_form(array $form, array &$form_state) {
  $form = array();

  $form['start_sync'] = array(
    '#type' => 'item',
    '#title' => t('Start synchronization'),
    '#description' => t('Start the synchronization process from the last date a successful synchronization was made or from a specific other date.'),
  );
  $form['last_synchronization_date'] = array(
    '#type' => 'textfield',
    '#title' => t('Last synchronization date'),
    '#default_value' => variable_get('islandora_metadata_synchronization_last_synchronization_date'),
    '#description' => t('Use a date with the following format: YYYY-MM-DD'),
  );
  $form['synchronize_start'] = array(
    '#type' => 'submit',
    '#value' => t('Start synchronization'),
    '#submit' => array('islandora_metadata_synchronization_start_sync_submit'),
  );
  $form['where_are_sources'] = array(
    '#type' => 'item',
    '#description' => t('Note: you can change the sources of the synchronization via the sources tab.'),
  );

  return $form;
}

/**
 * Implements hook_form().
 */
function islandora_metadata_synchronization_sources_form(array $form, array &$form_state) {
  module_load_include('inc', 'islandora_metadata_synchronization', 'includes/retrieval_oaipmh');

  $xsl_files = islandora_metadata_synchronization_xsl_files();
  $transform_options = array('' => t('No transformation selected'));
  $transform_options = array_merge($transform_options, $xsl_files);

  $form = array();

  $form['sources'] = array(
    '#type' => 'item',
    '#title' => t('Sources'),
    '#description' => t('The metadata synchronization can use multiple sources. When synchronizing, each source is tried in order. If a valid identifier is found and metadata can be retrieved with it using the retrieval type and url, other sources are skipped. In the synchronize page of a specific item you can see which source is used.'),
  );
  for ($i = 1; $i <= number_of_sources() + 1; $i++) {
    $form['group' . $i] = array(
      '#type' => 'fieldset',
      '#title' => t("Source @nr", array("@nr" => $i)),
    );
    if (isset($form_state['input']['source' . $i . '_oaipmh_url']) && strlen($form_state['input']['source' . $i . '_oaipmh_url']) > 0) {
      $oaipmh_url = $form_state['input']['source' . $i . '_oaipmh_url'];
    }
    else {
      $oaipmh_url = variable_get('islandora_metadata_synchronization_source' . $i . '_oaipmh_url');
    }
    $form['group' . $i]['oai'] = array(
      '#type' => 'fieldset',
      '#title' => t('Data source: OAI-PMH settings'),
    );
    $form['group' . $i]['oai']['source' . $i . '_oaipmh_url'] = array(
      '#type' => 'textfield',
      '#title' => t('Retrieval URL'),
      '#description' => t('URL of an end point that is capable of supplying data.'),
      '#default_value' => $oaipmh_url,
    );
    if (isset($oaipmh_url) && strlen($oaipmh_url) > 0) {
      $reponame = islandora_metadata_synchronization_test_oai_pmh_url($oaipmh_url);
      if (isset($reponame) && strlen($reponame) > 0) {
        $markup = format_string('<img src="@image_url"/>!message', array(
          '@image_url' => url('misc/watchdog-ok.png'),
          '!message' => t('Successfully connected to OAI-PMH server named @name.', array('@name' => $reponame)),
        ));
      }
      else {
        $markup = format_string('<img src="@image_url"/>!message', array(
          '@image_url' => url('misc/watchdog-error.png'),
          '!message' => t('The URL above is not a valid URL for an OAI-PMH server.'),
        ));
      }
      $form['group' . $i]['oai']['infobox'] = array(
        '#type' => 'item',
        '#markup' => $markup,
      );
    }

    $form['group' . $i]['oai']['source' . $i . '_set'] = array(
      '#type' => 'textfield',
      '#title' => t('Set'),
      '#default_value' => variable_get('islandora_metadata_synchronization_source' . $i . '_set'),
      '#description' => t('The specific set to use at this end point, leave empty to use no set.'),
    );
    $form['group' . $i]['oai']['source' . $i . '_mdprefix'] = array(
      '#type' => 'textfield',
      '#title' => t('Metadata prefix'),
      '#default_value' => variable_get('islandora_metadata_synchronization_source' . $i . '_mdprefix'),
      '#description' => t('The type of metadata that is used for retrieval, for example: oai_dc.'),
    );

    $form['group' . $i]['target'] = array(
      '#type' => 'fieldset',
      '#title' => t('Data target'),
    );
    $desc = t('The datastream that is used as the target for the data coming from the source.');
    if ($i == 1) {
      $desc .= ' ' . t('For example: MODS');
    }
    if ($i == 2) {
      $desc .= ' ' . t('For example: DC');
    }
    $form['group' . $i]['target']['source' . $i . '_target_dsid'] = array(
      '#type' => 'textfield',
      '#title' => t('Datastream ID'),
      '#default_value' => variable_get('islandora_metadata_synchronization_source' . $i . '_target_dsid'),
      '#description' => check_plain($desc),
    );
    $form['group' . $i]['target']['source' . $i . '_mdxsl'] = array(
      '#type' => 'select',
      '#title' => t('Stylesheet'),
      '#options' => $transform_options,
      '#default_value' => variable_get('islandora_metadata_synchronization_source' . $i . '_mdxsl'),
      '#description' => t('Stylesheet to use for transforming the OAI-record XML to the target datastream. See "Stylesheets" to add to the above list.'),
    );

    $form['group' . $i]['identifier'] = array(
      '#type' => 'fieldset',
      '#title' => t('Identifier location'),
    );
    $desc = t('The datastream that is used to retrieve the ID for synchronization.');
    if ($i == 1) {
      $desc .= ' ' . t('For example: MODS');
    }
    if ($i == 2) {
      $desc .= ' ' . t('For example: SYNCID');
    }
    $form['group' . $i]['identifier']['source' . $i . '_id_dsid'] = array(
      '#type' => 'textfield',
      '#title' => t('Datastream ID'),
      '#default_value' => variable_get('islandora_metadata_synchronization_source' . $i . '_id_dsid'),
      '#description' => check_plain($desc),
    );

    $desc = t('The XPath to search the datastream that is used to retrieve the ID for synchronization.');
    if ($i == 1) {
      $desc .= ' ' . t("For example: /mods:mods/mods:recordInfo/mods:recordIdentifier[@source='NL-LeU']");
    }
    if ($i == 2) {
      $desc .= ' ' . t('For example leave blank, if used with datastream ID SYNCID.');
    }
    $form['group' . $i]['identifier']['source' . $i . '_path'] = array(
      '#type' => 'textfield',
      '#title' => t('XPath ID'),
      '#default_value' => variable_get('islandora_metadata_synchronization_source' . $i . '_path'),
      '#description' => check_plain($desc),
    );
    $form['group' . $i]['identifier']['source' . $i . '_id_prefix'] = array(
      '#type' => 'textfield',
      '#title' => t('Prefix ID'),
      '#default_value' => variable_get('islandora_metadata_synchronization_source' . $i . '_id_prefix'),
      '#description' => t('The default prefix that is used if the identifier appears to be missing this prefix.'),
    );
  }
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save configuration'),
    '#submit' => array('islandora_metadata_synchronization_settings_form_submit'),
    '#validate' => array('islandora_metadata_synchronization_settings_form_validate'),
  );

  return $form;
}

/**
 * Implements hook_form_validate().
 */
function islandora_metadata_synchronization_settings_form_validate(array $form, array &$form_state) {
  module_load_include('inc', 'islandora_metadata_synchronization', 'includes/retrieval_oaipmh');
  module_load_include('inc', 'islandora_metadata_synchronization', 'includes/objects_identifiers');
  for ($i = 1; $i <= number_of_sources() + 1; $i++) {
    $fs = array();
    $fs['oaipmh_url'] = $form_state['values']['source' . $i . '_oaipmh_url'];
    $fs['set'] = $form_state['values']['source' . $i . '_set'];
    $fs['id_prefix'] = $form_state['values']['source' . $i . '_id_prefix'];
    $fs['target_dsid'] = $form_state['values']['source' . $i . '_target_dsid'];
    $fs['id_dsid'] = $form_state['values']['source' . $i . '_id_dsid'];
    $fs['path'] = $form_state['values']['source' . $i . '_path'];
    $fs['mdxsl'] = $form_state['values']['source' . $i . '_mdxsl'];
    $fs['mdprefix'] = $form_state['values']['source' . $i . '_mdprefix'];

    if (strlen($fs['oaipmh_url']) > 0) {
      $reponame = islandora_metadata_synchronization_test_oai_pmh_url($fs['oaipmh_url']);
      if ($reponame == FALSE) {
        form_set_error('source' . $i . '_oaipmh_url', t('Must be a valid OAI-PMH URL!'));
      }

      $mandatory = array(
        'mdprefix' => t('OAI-PMH metadata prefix'),
        'target_dsid' => t('Target datastream id'),
        'mdxsl' => t('Stylesheet'),
        'id_dsid' => t('Identifier datastream ID'),
      );
      foreach ($mandatory as $key => $value) {
        if (strlen($fs[$key]) == 0) {
          form_set_error('source' . $i . '_' . $key, t('Source !i should have a %field', array('!i' => $i, '%field' => $value)));
        }
      }

      if (strlen($fs['id_dsid']) == 0 && strlen($fs['path']) > 0) {
        form_set_error('source' . $i . '_path', t('Source !i has an xpath but not a datastream id', array('!i' => $i)));
      }
      elseif (strlen($fs['path']) > 0) {
        // Check xpath.
        $doc = new DOMDocument();
        $xpath = islandora_metadata_synchronization_new_xpath($doc);

        if (@$xpath->query($fs['path']) === FALSE) {
          form_set_error('source' . $i . '_path', t('Source !i has an invalid xpath', array('!i' => $i)));
        }
      }
    }
    else {
      $names = array(
        'set' => t('OAI-PMH Set'),
        'id_prefix' => t('Prefix ID'),
        'target_dsid' => t('Target datastream id'),
        'id_dsid' => t('Identifier datastream ID'),
        'path' => t('Identfier xpath'),
        'mdxsl' => t('Stylesheet'),
        'mdprefix' => t('OAI-PMH metadata prefix'),
      );
      foreach ($names as $key => $value) {
        if (strlen($fs[$key]) > 0) {
          form_set_error('source' . $i . '_' . $key, t('Source !i has a %field but not a retrieval url', array('!i' => $i, '%field' => $value)));
        }
      }
    }
  }
}

/**
 * Implements hook_form_submit().
 */
function islandora_metadata_synchronization_settings_form_submit(array $form, array &$form_state) {
  $vscounter = 1;
  for ($i = 1; $i <= 99; $i++) {
    if (array_key_exists('source' . $i . '_oaipmh_url', $form_state['values']) && strlen($form_state['values']['source' . $i . '_oaipmh_url']) > 0) {
      variable_set('islandora_metadata_synchronization_source' . $vscounter . '_oaipmh_url', $form_state['values']['source' . $i . '_oaipmh_url']);
      variable_set('islandora_metadata_synchronization_source' . $vscounter . '_set', $form_state['values']['source' . $i . '_set']);
      variable_set('islandora_metadata_synchronization_source' . $vscounter . '_id_prefix', $form_state['values']['source' . $i . '_id_prefix']);
      variable_set('islandora_metadata_synchronization_source' . $vscounter . '_target_dsid', $form_state['values']['source' . $i . '_target_dsid']);
      variable_set('islandora_metadata_synchronization_source' . $vscounter . '_id_dsid', $form_state['values']['source' . $i . '_id_dsid']);
      variable_set('islandora_metadata_synchronization_source' . $vscounter . '_path', $form_state['values']['source' . $i . '_path']);
      variable_set('islandora_metadata_synchronization_source' . $vscounter . '_mdxsl', $form_state['values']['source' . $i . '_mdxsl']);
      variable_set('islandora_metadata_synchronization_source' . $vscounter . '_mdprefix', $form_state['values']['source' . $i . '_mdprefix']);
      $vscounter += 1;
    }
  }
  for ($i = $vscounter; $i <= 99; $i++) {
    variable_del('islandora_metadata_synchronization_source' . $i . '_oaipmh_url');
    variable_del('islandora_metadata_synchronization_source' . $i . '_set');
    variable_del('islandora_metadata_synchronization_source' . $i . '_id_prefix');
    variable_del('islandora_metadata_synchronization_source' . $i . '_target_dsid');
    variable_del('islandora_metadata_synchronization_source' . $i . '_id_dsid');
    variable_del('islandora_metadata_synchronization_source' . $i . '_path');
    variable_del('islandora_metadata_synchronization_source' . $i . '_mdxsl');
    variable_del('islandora_metadata_synchronization_source' . $i . '_mdprefix');
  }
  drupal_set_message(t('The settings have been saved!'));
}

/**
 * Implements hook_form_submit().
 */
function islandora_metadata_synchronization_start_sync_submit(array $form, array &$form_state) {
  module_load_include('inc', 'islandora_metadata_synchronization', 'includes/synchronize');
  $date = $form_state['values']['last_synchronization_date'];

  islandora_metadata_synchronization_start_sync(array('date' => $date));
}


/**
 * Returns xsl files used for transform.
 */
function islandora_metadata_synchronization_xsl_files() {
  $files = file_scan_directory(drupal_get_path('module', 'islandora_metadata_synchronization') . '/xsl/', '/.*\.xslt?$/');

  $transform_options = array();

  foreach ($files as $file) {
    $transform_options[$file->uri] = $file->filename;
  }

  $oai_uploaded_files = array();
  $upload_path = 'public://islandora_metadata_synchronization_xsls';
  $uploaded_files = file_scan_directory($upload_path, '/.*\.xslt?$/');

  foreach ($uploaded_files as $up_file) {
    $oai_uploaded_files[$up_file->uri] = $up_file->filename;
  }

  foreach ($oai_uploaded_files as $key => $file) {
    if (!in_array($file, $transform_options)) {
      $transform_options[$key] = $file;
    }
  }
  return $transform_options;
}
