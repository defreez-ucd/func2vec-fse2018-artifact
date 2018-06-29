#!/usr/bin/env python3

import os
import sys
import argparse
import ehnfer.commands

def main():
    script_dir = os.path.dirname(os.path.realpath(__file__))

    action, kwargs = parse_args()
    action(**kwargs)


def parse_args():
    def help_and_exit(parser):
        parser.print_help()
        sys.exit()

    parser = argparse.ArgumentParser(formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('command', type=str, nargs='?',
                        help="Action to run. Possible commands:\n"
                             "mine\t\trequires --db, can pass sthresh here\n"
                             "\t\t\tpassing --input will copy labels from input file to report\n"
                             "stripnumbers\trequires --db\n"
                             "killnumbers\trequires --db\n"
                        )
    parser.add_argument('--input', type=str,
                        help='Path to input file.')
    parser.add_argument('--output', type=str,
                        help='Path to output file. None for stdout.')
    parser.add_argument('--db', type=str,
                        help='Path to database.')
    parser.add_argument('--support', type=str,
                        help="Support threshold, defaults to 10", required=True)
    parser.add_argument('--similarity', type=str,
                        help="Similarity threshold, defaults to 0.9", required=True)
    parser.add_argument('--model', type=str, help="Path to model file", required=True)

    args = parser.parse_args()

    # Default options
    arg_command = None

    if args.command:
        arg_command = args.command
    else:
        help_and_exit(parser)

    action_kwargs = dict()
    if arg_command == "mine":
        if not args.db:
            help_and_exit(parser)

        action = ehnfer.commands.mine
        action_kwargs["db_file"] = args.db
        action_kwargs["support_threshold"] = int(args.support)
        action_kwargs["similarity_threshold"] = float(args.similarity)
        action_kwargs["model_file"] = args.model
    else:
        help_and_exit(parser)

    return action, action_kwargs

if (__name__ == "__main__"):
    main()
