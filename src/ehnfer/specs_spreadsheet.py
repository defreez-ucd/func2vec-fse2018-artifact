from apiclient.discovery import build
from httplib2 import Http
from oauth2client import file, client, tools
import argparse

class SpecsSpreadsheet(object):
    """Handles auth/requests for a single login + spreadsheet
    """
    def __init__(self, creds_file, spreadsheet_id):
        self.service = self.login(creds_file, spreadsheet_id)
        self.spreadsheet_id = spreadsheet_id

    def login(self, creds_file, spreadsheet_id):
        """Logs in to the google sheets API.
        Returns service.
        """

        creds = None

        # If modifying these scopes, delete the file token.json
        SCOPES = 'https://www.googleapis.com/auth/spreadsheets'

        store = file.Storage('token.json')
        try:
            creds = store.get()
        except:
            pass

        if not creds or creds.invalid:
            # TODO: Will need to parameterize credentials.json
            flow = client.flow_from_clientsecrets(creds_file, SCOPES)
            flags=tools.argparser.parse_args(args=['--noauth_local_webserver'])
            creds = tools.run_flow(flow, store, flags)
        service = build('sheets', 'v4', http=creds.authorize(Http()))
        return service

    def get(self, range):
        request = self.service.spreadsheets().values().get(
            spreadsheetId=self.spreadsheet_id, range=range)
        response = request.execute()
        values = response.get('values', [])
        return values

    def clear(self, range):
        request = self.service.spreadsheets().values().clear(
            spreadsheetId=self.spreadsheet_id,
            range=range)
        response = request.execute()

    def bulk_add_specs(self, range, specs):
        """
        range: A1 range of sheet
        specs: list of Spec namedtuples
        """
        values = []
        for s in specs:
            context = ", ".join(s.rule.context)
            response = ", ".join(s.rule.response)
            value = [context, response, s.eclat_support, s.support, s.merged_support,
                     s.global_confidence, s.local_confidence]
            values.append(value)

        value_range_body = {
            'values': values
        }

        request = self.service.spreadsheets().values().append(
            spreadsheetId=self.spreadsheet_id, range=range,
            valueInputOption='RAW',
            insertDataOption='OVERWRITE',
            body=value_range_body)
        response = request.execute()


    def add_spec(self, range, specs):
        """
        association_rule: an individual association_rule object
        rules: A list of Spec named tuples
        """
        # association_rule, eclat_support, support, merged_support, global_confidence, local_confidence):
        context = ", ".join(association_rule.context)
        response = ", ".join(association_rule.response)
        values = [
            [
                context, response, eclat_support, support, merged_support, global_confidence, local_confidence
            ]
        ]

        value_range_body = {
            'values': values
        }

        request = self.service.spreadsheets().values().append(
            spreadsheetid=self.spreadsheet_id, range=range,
            valueinputoption='RAW',
            insertdataoption='OVERWRITE',
            body=value_range_body)
        response = request.execute()

