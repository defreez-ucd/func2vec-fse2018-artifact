import sqlite3
from enum import Enum
from collections import Counter
import itertools


class ItemType(Enum):
    CALL = 1
    STORE = 2
    LOAD = 3

    @staticmethod
    def from_string(string):
        if string == "CALL":
            return ItemType.CALL
        elif string == "STORE":
            return None
        elif string == "LOAD":
            return None
        elif string == "EC":
            return None
        else:
            raise Exception("Unknown tactic: %s" % string)


class TacticType(Enum):
    PRE = 1
    POST = 2
    FN = 3
    HNDL = 4

    @staticmethod
    def from_string(string):
        if string == "PRE":
            return TacticType.PRE
        elif string == "POST":
            return TacticType.POST
        elif string == "FN":
            return None
        elif string == "HNDL":
            return None
        else:
            raise Exception("Unknown tactic: %s" % string)


class Item:
    def __init__(self, name, item_type, tactic_type):
        assert (isinstance(tactic_type, TacticType))
        assert (isinstance(item_type, ItemType))

        self.name = unicode(name)
        self.item_type = item_type
        self.tactic_type = tactic_type
        self.other_names = set()

    @staticmethod
    def from_string(string):
        try:
            tactic_type_str, item_name, item_type_str = string.split("|")
        except:
            pass
        return Item(item_name, ItemType.from_string(item_type_str), TacticType.from_string(tactic_type_str))

    def __eq__(self, other):
        return self.name == other.name and self.item_type == other.item_type and self.tactic_type == other.tactic_type

    def __hash__(self):
        return hash((self.name, self.item_type.name, self.tactic_type.name))

    def __repr__(self):
        return self.name


class ErrorHandler:
    def __init__(self, db_id, stack, predicate_loc, parent_function):
        self.db_id = db_id
        self.stack = stack
        self.predicate_loc = predicate_loc
        self.context = []  # List of items
        self.response = []  # List of items
        self.parent_function = parent_function

        self.supported_specs = set()

    def __repr__(self):
        return self.predicate_loc

    def __eq__(self, other):
        return self.predicate_loc == other.predicate_loc

    def __hash__(self):
        return hash(self.predicate_loc)

    def supports(self, spec):
        """Does this handler support the specification?"""
        if not spec.applies_to(self):
            return False
        return not self.violates(spec)

    def violates(self, spec):
        """Does this handler violate the specification?"""
        if not spec.applies_to(self):
            return False

        missing = self.missing_items(spec)
        tmp = list(missing)
        # Ignore tactic_type (PRE vs. POST)
        for i in tmp:
            for j in self.context:
                if i in missing and i.name == j.name and i.item_type == j.item_type:
                    missing.remove(i)

        return len(missing) != 0

    def elixir_predicate(self):
        import string
        tmp = string.replace(self.predicate_loc, ':', '#L')
        return tmp

    def missing_items(self, spec):
        """Given the specification, What items are missing from this handler?
        If the specification does not apply, or if the handler does not
        violate the specification, then an empty list is returned.

        Parameters
        ----------
        spec: A Spec object

        Returns
        -------
        A list of Item objects
        """
        if not spec.applies_to(self):
            return []
        return spec.response.difference(self.response)

    def merge(self, item_to_merged):
        """
        Merges the items in a handler
        :param item_to_merged Map from an individual item to the representative name used after merging
        :return:
        """

        def do_merge(item):
            if item.name in item_to_merged:
                new_item = Item(item_to_merged[item.name], item.item_type, item.tactic_type)
                return new_item
            return item

        after_merge = ErrorHandler(self.db_id, self.stack, self.predicate_loc, self.parent_function)
        after_merge.context = map(do_merge, self.context)
        after_merge.response = map(do_merge, self.response)
        return after_merge


class Spec(object):
    def __init__(self, db_id=None):
        self.db_id = db_id  # Database id. Can be none if this hasn't been written to a DB.
        self.__confidence = None  # Computed on property access

        self.applicable_handlers = set()  # Handlers this specification applies to
        self.supporting_handlers = set()  # Handlers that support this specification
        self.supporting_specs = set()  # For merged specs only
        self.function_support = 0
        self.rewritten = False
        self.merged = False

        self.context = set()  # Set of Items
        self.response = set()  # Set of Items

        self.apply_cache = dict()

    def __repr__(self):
        # Go from set() to {}
        context_str = "{%s}" % ', '.join(str(x) for x in self.context)
        response_str = "{%s}" % ', '.join(str(x) for x in self.response)
        return "%s -> %s" % (context_str, response_str)

    def applies_to(self, handler):
        """Does this specification apply to the handler?"""

        if handler in self.apply_cache:
            return self.apply_cache[handler]

        ret = False
        if not self.context.issubset(handler.context):
            ret = False
        else:
            trans_ctx = Counter(handler.context)
            full_spec = Counter(self.context) + Counter(self.response)
            for x in full_spec:
                if full_spec[x] > trans_ctx[x]:
                    ret = True
                    break

        self.apply_cache[handler] = ret
        return ret

    @staticmethod
    def from_str(str):
        # Extract string representations of individual items
        context_str_list = str[1:str.find('}')].split(',')
        context_str_list = [x.strip() for x in context_str_list]
        response_str_list = str[str.rfind('{') + 1:-1].split(',')
        response_str_list = [x.strip() for x in response_str_list]

        # Convert string representation of items to actual objects
        context_items = []
        for item_str in context_str_list:
            tokenized = item_str.split('|')
            tactic_type = TacticType.from_string(tokenized[0])
            if not tactic_type:
                continue
            name = tokenized[1]
            item_type = ItemType.from_string(tokenized[2])
            if not item_type:
                continue
            i = Item(name, item_type, tactic_type)
            context_items.append(i)

        response_items = []
        for item_str in response_str_list:
            tokenized = item_str.split('|')
            tactic_type = TacticType.from_string(tokenized[0])
            if not tactic_type:
                continue
            name = tokenized[1]
            item_type = ItemType.from_string(tokenized[2])
            if not item_type:
                continue
            i = Item(name, item_type, tactic_type)
            response_items.append(i)

        spec = Spec(None)
        spec.context = set(context_items)
        spec.response = set(response_items)

        return spec

    def __eq__(self, other):
        return self.context == other.context and self.response == other.response

    def __hash__(self):
        context_and_actions = (frozenset(self.context), frozenset(self.response))
        return hash(context_and_actions)

    @property
    def support(self):
        return len(self.supporting_handlers)

    @property
    def applies(self):
        return len(self.applicable_handlers)

    @property
    def violating_handler_ids(self):
        return self.applicable_handlers.difference(self.supporting_handlers)

    @property
    def confidence(self):
        #        assert self.__confidence or self.applies > 0
        if not self.__confidence and self.applies == 0:
            return 0.0
        if self.__confidence:
            return self.__confidence
        return float(self.support) / (self.applies)

    def similarity(self, word_vectors):
        merged_interchangeable_items = self.merged_interchangeable_items()
        num_items = len(list(itertools.combinations(merged_interchangeable_items, 2)))
        if num_items == 0:
            return 1.0

        total_sim = 0.0
        for x, y in itertools.combinations(merged_interchangeable_items, 2):
            sim = word_vectors.similarity(x, y)
            total_sim += sim
        sim = total_sim / num_items
        return sim


class HandlerDb:
    def __init__(self, path):
        self.conn = sqlite3.connect(path)
        self.cur = self.conn.cursor()

        # Map database id to handler/spec objects
        self.handlers = dict()
        self.raw_handlers = self.handlers
        self.merged_handlers = dict()
        self.merged_to_itemname = dict()
        self.frozenset_to_merged = dict()
        self.itemname_to_merged = dict()
        self.specs = dict()  # spec_id -> spec
        self.spec_ids = dict()  # spec -> spec_id

        self.read_handlers()

    def read_handlers(self):
        from collections import defaultdict

        query = "SELECT id, stack, predicate_loc, parent_function FROM Handler"
        res = self.cur.execute(query)
        for row in res:
            handler = ErrorHandler(row[0], row[1], row[2], row[3])
            self.handlers[row[0]] = handler

        query = "SELECT handler, item, type, tactic FROM Context"
        res = self.cur.execute(query)
        for row in res:
            item_type = ItemType.from_string(row[2])
            if not item_type:
                continue
            tactic_type = TacticType.from_string(row[3])
            if not tactic_type:
                continue
            item = Item(row[1], item_type, tactic_type)
            self.handlers[row[0]].context.append(item)

        query = "SELECT handler, item, type, tactic FROM Response"
        res = self.cur.execute(query)
        for row in res:
            item_type = ItemType.from_string(row[2])
            if not item_type:
                continue
            tactic_type = TacticType.from_string(row[3])
            if not tactic_type:
                continue
            item = Item(row[1], item_type, tactic_type)
            self.handlers[row[0]].response.append(item)

        # Preprocess to remove PRE items from context if same FN item exists
        for k in self.handlers:
            handler = self.handlers[k]
            returning_error = set()
            for i in handler.context:
                if (i.tactic_type == TacticType.FN):
                    returning_error.add(i.name)
            handler.context = [i for i in handler.context if
                               not (i.name in returning_error and i.tactic_type == TacticType.PRE)]

    def read_specs(self):
        query = "SELECT id FROM Spec"
        res = self.cur.execute(query)
        for row in res:
            spec = Spec(row[0])
            self.specs[row[0]] = spec

        query = "SELECT spec, action FROM SpecContext"
        res = self.cur.execute(query)
        for row in res:
            tactic_str, name, type_str = row[1].split('|')
            item = Item(name, ItemType.from_string(type_str), TacticType.from_string(tactic_str))
            self.specs[row[0]].context.add(item)

        query = "SELECT spec, action FROM SpecAction"
        res = self.cur.execute(query)
        for row in res:
            tactic_str, name, type_str = row[1].split('|')
            item = Item(name, ItemType.from_string(type_str), TacticType.from_string(tactic_str))
            self.specs[row[0]].response.add(item)

        query = "SELECT spec, handler, violation FROM Results"
        res = self.cur.execute(query)
        for row in res:
            spec_id = row[0]
            handler_id = row[1]
            violation = row[2]
            self.specs[spec_id].applicable_handlers.add(handler_id)
            if not violation:
                self.specs[spec_id].supporting_handlers.add(handler_id)

        query = "SELECT spec, supporting FROM MergedSpec"
        res = self.cur.execute(query)
        for row in res:
            spec_id = row[0]
            supporting_spec_id = row[1]
            self.specs[spec_id].add_supporting_spec(self.specs[supporting_spec_id])
            self.specs[spec_id].parse_merges(self.merged_to_itemname)

    # Caching wrapper around .applies_to()
    def is_spec_applicable(self, spec, handler):
        if (spec, handler) in apply_cache:
            return apply_cache[(spec, handler)]
        ret = spec.applies_to(handler)
        apply_cache[(spec, handler)] = ret
        return ret

    # Returns list of handlers such that the responses contain
    def handlers_containing_response(self, item):
        ret = []
        for k in self.handlers:
            handler = self.handlers[k]
            if item in handler.response:
                ret.append(handler)

        return ret

    def specs_containing_response(self, item):
        ret = []
        for k in self.specs:
            spec = self.specs[k]
            if item in spec.response:
                ret.append(spec)

        return ret

    def all_specs(self):
        """List all of the specifications.

        Returns
        -------
        A list of all specifications in the database.
        The database must be mined first.
        """
        return self.specs

    def flatten(self):
        """
        Return all of the transactions as a flat database.
        :return:
        """
        flat_transactions = []
        for t in self.handlers:
            flat_transactions.append(self.handlers[t].context + self.handlers[t].response)
        return flat_transactions

    def supporting(self, spec):
        """List all of the handlers that support a specification.

        Parameters
        ----------
        spec: A Spec object

        Returns
        -------
        A list of handlers
        """
        return [self.handlers[k] for k in self.handlers if self.handlers[k].supports(spec)]

    def violating(self, spec):
        """List all of handlers that violate a specification.

        Parameters
        ----------
        spec: A Spec object

        Returns
        -------
        A list of handlers
        """
        return [self.handlers[k] for k in self.handlers if self.handlers[k].violates(spec)]

    def is_cross_impl(self, spec):
        seen = set()
        handlers = spec.supporting_handlers
        for h in handlers:
            handler = self.handlers[h]
            impl = handler.predicate_loc.split('/')[1]
            seen.add(impl)
        return len(seen) > 1

    def create_spec_tables(self):
        self.conn.execute("CREATE TABLE Spec (id INTEGER PRIMARY KEY NOT NULL, support INTEGER, confidence REAL)")
        self.conn.execute("CREATE TABLE SpecContext (spec INTEGER NOT NULL, action TEXT NOT NULL)")
        self.conn.execute("CREATE TABLE SpecAction (spec INTEGER NOT NULL, action TEXT NOT NULL)")
        self.conn.execute("CREATE TABLE Results (spec INTEGER, handler INTEGER, violation INTEGER)")
        self.conn.execute("CREATE TABLE MergedSpec (spec INTEGER, supporting INTEGER)")
        self.conn.commit()

    def drop_spec_tables(self):
        self.conn.execute("DROP TABLE IF EXISTS Spec")
        self.conn.execute("DROP TABLE IF EXISTS SpecContext")
        self.conn.execute("DROP TABLE IF EXISTS SpecAction")
        self.conn.execute("DROP TABLE IF EXISTS Results")
        self.conn.execute("DROP TABLE IF EXISTS MergedSpec")
        self.conn.execute("DROP TABLE IF EXISTS MergedItem")
        self.conn.commit()

    def table_exists(self, table_name):
        query = "SELECT Name FROM sqlite_master WHERE type='table' AND name=?"
        c = self.conn.cursor()
        c.execute(query, (table_name,))
        res = c.fetchone()

        if res:
            return True
        else:
            return False

    @property
    def spec_table_exists(self):
        return self.table_exists("Spec")

    def add_spec(self, spec):
        """Writes a single specification to the database.
        """
        for i in spec.context.union(spec.response):
            if i.name in self.frozenset_to_merged:
                i.name = self.frozenset_to_merged[i.name]

        # Create an id for the specification
        c = self.conn.cursor()
        query = "INSERT INTO Spec (id, support, confidence) VALUES (NULL, ?, ?)"
        c.execute(query, (spec.support, spec.confidence))
        spec_id = c.lastrowid

        # Insert context actions
        for item in spec.context:
            query = "INSERT INTO SpecContext (spec, action) VALUES (?, ?)"
            c.execute(query, (spec_id, str(item)))

        # Insert actions
        for item in spec.response:
            query = "INSERT INTO SpecAction (spec, action) VALUES (?, ?)"
            c.execute(query, (spec_id, str(item)))

        self.specs[spec_id] = spec
        self.spec_ids[spec] = spec_id

        self.conn.commit()

    def add_merged_spec(self, spec):
        c = self.conn.cursor()

        for i in spec.context:
            if i.name in self.itemname_to_merged.keys():
                i.name = self.itemname_to_merged[i.name]
        for i in spec.response:
            if i.name in self.itemname_to_merged.keys():
                i.name = self.itemname_to_merged[i.name]

        # Merged specs must already exist in the database
        assert spec in self.spec_ids
        for s in spec.supporting_specs:
            assert s in self.spec_ids
            query = "INSERT INTO MergedSpec (spec, supporting) VALUES (?, ?)"
            c.execute(query, (self.spec_ids[spec], self.spec_ids[s]))

        self.conn.commit()

    def write_results(self):
        """
        The results table contains information about which handlers support/violate a specification.
        """
        c = self.conn.cursor()
        for spec_db_id in self.specs:
            spec = self.specs[spec_db_id]

            for h in spec.supporting_handlers:
                query = "INSERT INTO Results (spec, handler, violation) VALUES (?,?,?)"
                c.execute(query, (spec_db_id, h.db_id, 0))

            violating_handlers = spec.applicable_handlers.difference(spec.supporting_handlers)
            for h in violating_handlers:
                query = "INSERT INTO Results (spec, handler, violation) VALUES (?,?,?)"
                c.execute(query, (spec_db_id, h.db_id, 1))

        self.conn.commit()

    def merge(self, function_classes=None):
        """
        :param function_classes: A list of hashable sets of interchangeable functions
        :return:
        """
        # Maps item name to the representative name to use, given function_classes

        id_counter = 0
        for bootstrap_set in function_classes:
            name = "FUNC2VEC_%d" % id_counter
            for i in bootstrap_set:
                self.itemname_to_merged[i] = name
            self.merged_to_itemname[name] = bootstrap_set
            self.frozenset_to_merged[bootstrap_set] = name
            id_counter += 1

        self.merged_handlers = {k: v.merge(self.itemname_to_merged) for k, v in self.handlers.iteritems()}
        self.handlers = self.merged_handlers

        c = self.conn.cursor()
        c.execute("CREATE TABLE MergedItem (merged TEXT, unmerged TEXT)")
        for k, v in self.itemname_to_merged.iteritems():
            query = "INSERT INTO MergedItem (merged, unmerged) VALUES (?, ?)"
            c.execute(query, (v, k))
        self.conn.commit()
