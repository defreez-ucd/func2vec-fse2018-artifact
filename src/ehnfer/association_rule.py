class AssociationRule(object):
    # For creating an unmerged rule
    def __init__(self, context, response):
        # === Immutable properties ===
        self.__context = context
        self.__response = response

    def __repr__(self):
        context = ", ".join(self.context)
        response = ", ".join(self.response)
        return "{{{}}} -> {{{}}}".format(context, response)

    def __eq__(self, other):
        return self.context == other.context and self.response == other.response

    def __hash__(self):
        return hash((frozenset(self.context), frozenset(self.response)))

    @property
    def context(self):
        return self.__context

    @property
    def response(self):
        return self.__response

