import ast
import operator

_OP_MAP = {
    ast.Add: operator.add,
    ast.Sub: operator.sub,
    ast.Mult: operator.mul,
    ast.Div: operator.truediv,
    ast.Invert: operator.neg,
}


class Calc(ast.NodeVisitor):
    def __init__(self) -> None:
        self.opCnt = 0
        super().__init__()

    def visit_BinOp(self, node):
        self.opCnt += 1
        left = self.visit(node.left)
        right = self.visit(node.right)
        return _OP_MAP[type(node.op)](left, right)

    def visit_Num(self, node):
        return node.n

    def visit_Expr(self, node):
        return self.visit(node.value)

    @classmethod
    def eval(cls, expression):
        tree = ast.parse(expression)
        calc = cls()
        return calc.visit(tree.body[0])

    @classmethod
    def cntOpEval(cls, expression):
        tree = ast.parse(expression)
        calc = cls()
        val = calc.visit(tree.body[0])
        return calc.opCnt, val




