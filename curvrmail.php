#!/usr/bin/php
<?php

#
# curvr
# Richard Crowley <r@rcrowley.org>
#

# This works with my Nokia N73.  I don't care if it works on your phone.
# Tread lightly.

# How should we upload?
$UPLOAD_METHOD = 'email'; # or 'api'

# Secret email address from Flickr
#   Used if UPLOAD_METHOD is 'email'
$FLICKRMAIL = 'secret-flickrmail-address-goes-here';

# Flickr API token
#   Used if UPLOAD_METHOD is 'api'
$FLICKR_TOKEN = 'flickr-api-token-goes-here';

# Dopplr API token
#   If set, photos will be geotagged using FLICKR_TOKEN
#   UPLOAD_METHOD must be 'api' if this is set
$DOPPLR_TOKEN = 'dopplr-api-token-goes-here';

$CURVR_BIN = '/home/rcrowley/bin/curvr';
$CURVR_PROCESS = 'curve';
$CURVR_VERSION = '0.2';

# Tags to add to every photo
$TAGS = "curvr curvr:process=$CURVR_PROCESS curvr:version=$CURVR_VERSION";

# Get the raw email body from procmail
$raw = file_get_contents('php://stdin');
file_put_contents('/tmp/debug_' . time(), $raw);

# Process each line
$lines = explode("\n", $raw);
$boundary = false;
$content_type = false;
$blank = false;
$body = array();
$jpeg = array();
foreach ($lines as $line) {

	# First we have to find the boundary
	if (false === $boundary && '--' == substr($line, 0, 2)) {
		$boundary = $line;
	}

	# Seeing another boundary indicates the end of a part
	else if ($boundary == $line) {
		$content_type = false;
		$blank = false;
	}

	# Note the content type if this line specifies it
	else if (preg_match('/^Content-Type: ([a-z\/]+).*$/', $line, $match)) {
		$content_type = $match[1];
	}

	# Now find the blank line that indicates the beginning of content
	else if (!$blank && 0 == strlen(trim($line))) {
		$blank = true;
	}

	# The body
	else if ($blank && 'text/plain' == $content_type) {
		$body[] = $line;
	}

	# The JPEG attachment
	else if ($blank && 'image/jpeg' == $content_type) {
		$jpeg[] = $line;
	}

}

# Save the JPEG to be processed and emailed again
$ts = time();
@unlink("/tmp/curvrmail.$ts.jpg");
file_put_contents("/tmp/curvrmail.$ts.jpg",
	base64_decode(implode('', $jpeg)), 0);

# Run curvr on the JPEG
`$CURVR_BIN /tmp/curvrmail.$ts.jpg $CURVR_PROCESS /tmp/curvrmail.{$ts}_curve.jpg`;

# Get location info from Dopplr
#   At a bear minimum, use the city and non-US country as tags
$tags = '';
$geo = array();
if (isset($DOPPLR_TOKEN) && '' != $DOPPLR_TOKEN) {
	$xml = simplexml_load_file(
		"https://www.dopplr.com/api/traveller_info?token=$DOPPLR_TOKEN");
	$tags = strtolower(preg_replace('/\s+/', '',
		(string)$xml->traveller->current_city->name));
	if ('US' != (string)$xml->traveller->current_city->country_code) {
		$tags .= ' ' . strtolower(preg_replace('/\s+/', '',
			(string)$xml->traveller->current_city->country));
	}
	$geo['lat'] = (double)$xml->traveller->current_city->latitude;
	$geo['long'] = (double)$xml->traveller->current_city->longitude;
	if ((bool)$xml->traveller->travel_today) {
		$tags .= ' dopplr:trip=' . (string)$xml->traveller->current_trip->id;
	}
}

# Email to Flickr
if ('email' == $UPLOAD_METHOD) {
	$subject = implode(' ', $body);
	if (false === strpos($subject, 'tags:')) {
		$subject .= ' tags:';
	}
	mail($FLICKRMAIL, "$subject $tags $TAGS",
		"\r\n--deadbeef-for-dinner\r\n" .
		"Content-Type: text/plain; " .
		"charset=\"utf-8\"\r\n" .
		"Content-Transfer-Encoding: 7bit\r\n\r\n" .
		"\r\n\r\n--deadbeef-for-dinner\r\n" .
		"Content-Type: image/jpeg; filename=\"photo.jpg\"\r\n" .
		"Content-Transfer-Encoding: base64\r\n" .
		"Content-Disposition: attachment\r\n\r\n" .
		base64_encode(file_get_contents("/tmp/curvrmail.{$ts}_curve.jpg")) .
		"\r\n\r\n--deadbeef-for-dinner--\r\n",
		"From: Nobody <nobody@rcrowley.org>\r\n" .
		"MIME-Version: 1.0\r\n" .
		"Content-Type: multipart/mixed; " .
		"boundary=\"deadbeef-for-dinner\"\r\n" .
		"Content-Transfer-Encoding: 7bit\r\n");
}

# Upload using the API
else if ('api' == $UPLOAD_METHOD) {

	# Upload
	###

	# Really geotag
	###

}

?>
