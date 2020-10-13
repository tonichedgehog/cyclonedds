/*
 * Copyright(c) 2020 ADLINK Technology Limited and others
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v. 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
 * v. 1.0 which is available at
 * http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "idl/tree.h"

#include "CUnit/Theory.h"

/* a union must have at least one case */
CU_Test(idl_union, no_case)
{
  idl_retcode_t ret;
  idl_tree_t *tree = NULL;

  const char str[] = "union u switch(char) { };";
  ret = idl_parse_string(str, 0u, &tree);
  CU_ASSERT_EQUAL(ret, IDL_RETCODE_SYNTAX_ERROR);
  CU_ASSERT_PTR_NULL(tree);
  idl_delete_tree(tree);
}

CU_Test(idl_union, single_case)
{
  idl_retcode_t ret;
  idl_tree_t *tree = NULL;
  idl_union_t *u;
  idl_case_t *c;

  const char str[] = "union u switch(long) { case 1: char c; };";
  ret = idl_parse_string(str, 0u, &tree);
  CU_ASSERT_EQUAL_FATAL(ret, IDL_RETCODE_OK);
  u = (idl_union_t *)tree->root;
  CU_ASSERT_FATAL(idl_is_union(u));
  CU_ASSERT(idl_is_type_spec(u->switch_type_spec, IDL_LONG));
  c = (idl_case_t *)u->cases;
  CU_ASSERT_FATAL(idl_is_case(c));
  CU_ASSERT_PTR_EQUAL(idl_parent(c), u);
  CU_ASSERT(idl_is_case_label(c->case_labels));
  CU_ASSERT(idl_is_type_spec(c->type_spec, IDL_CHAR));
  CU_ASSERT_FATAL(idl_is_declarator(c->declarator));
  CU_ASSERT_STRING_EQUAL(idl_identifier(c->declarator), "c");
  c = idl_next(c);
  CU_ASSERT_PTR_NULL(c);
  idl_delete_tree(tree);
}

CU_Test(idl_union, single_default_case)
{
  idl_retcode_t ret;
  idl_tree_t *tree = NULL;
  idl_union_t *u;
  idl_case_t *c;

  const char str[] = "union u switch(char) { default: char c; };";
  ret = idl_parse_string(str, 0u, &tree);
  CU_ASSERT_EQUAL_FATAL(ret, IDL_RETCODE_OK);
  u = (idl_union_t *)tree->root;
  CU_ASSERT_FATAL(idl_is_union(u));
  CU_ASSERT(idl_is_type_spec(u->switch_type_spec, IDL_CHAR));
  c = (idl_case_t *)u->cases;
  CU_ASSERT_FATAL(idl_is_case(c));
  CU_ASSERT_PTR_EQUAL(idl_parent(c), u);
  CU_ASSERT(idl_is_default_case(c));
  CU_ASSERT(idl_is_type_spec(c->type_spec, IDL_CHAR));
  CU_ASSERT_FATAL(idl_is_declarator(c->declarator));
  CU_ASSERT_STRING_EQUAL(idl_identifier(c->declarator), "c");
  c = idl_next(c);
  CU_ASSERT_PTR_NULL(c);
  idl_delete_tree(tree);
}

// x. union with same declarators
// x. forward declared union
//   x.x. forward declared union before definition
//   x.x. forward declared union after definition
//   x.x. forward declared union with no definition at all
// x. forward declared struct
//   x.x. see union
// x. constant expressions
// x. identifier that collides with a keyword
// x. union with default
// x. union with two default branches
// x. union with multile labels for branch
// x. union with enumeration A and an enumerator from enumeration B

CU_Test(idl_union, enumerator_switch_type)
{
  idl_retcode_t ret;
  idl_tree_t *tree = NULL;
  idl_enum_t *e;
  idl_enumerator_t *el;
  idl_union_t *u;
  idl_case_t *c;
  const char *str;

  str = "enum Color { Red, Yellow, Blue };\n"
        "union u switch(Color) { case Red: char c; default: long l; };";

  ret = idl_parse_string(str, 0u, &tree);
  CU_ASSERT_EQUAL_FATAL(ret, IDL_RETCODE_OK);
  CU_ASSERT_PTR_NOT_NULL(tree);
  e = (idl_enum_t *)tree->root;
  CU_ASSERT_FATAL(idl_is_enum(e));
  el = e->enumerators;
  CU_ASSERT_FATAL(idl_is_enumerator(el));
  CU_ASSERT_STRING_EQUAL(idl_identifier(el), "Red");
  el = idl_next(el);
  CU_ASSERT_FATAL(idl_is_enumerator(el));
  CU_ASSERT_STRING_EQUAL(idl_identifier(el), "Yellow");
  el = idl_next(el);
  CU_ASSERT_FATAL(idl_is_enumerator(el));
  CU_ASSERT_STRING_EQUAL(idl_identifier(el), "Blue");
  u = (idl_union_t *)idl_next(e);
  CU_ASSERT_FATAL(idl_is_union(u));
  c = u->cases;
  CU_ASSERT_FATAL(idl_is_case(c));
  CU_ASSERT((uintptr_t)c->case_labels->const_expr == (uintptr_t)e->enumerators);
  idl_delete_tree(tree);
}

/* the type for the union discriminator must be an integer, char, boolean,
   enumeration, or a reference to one of these */
#define M(name, definitions) "module " name " { " definitions " };"
#define S(name) "struct " name " { char c; };"
#define T(type, name) "typedef " type " " name ";"
#define U(type) "union u switch (" type ") { default: char c; };"

CU_Test(idl_union, typedef_switch_types)
{
  idl_retcode_t ret;
  idl_tree_t *tree = NULL;
  idl_module_t *m;
  idl_typedef_t *t;
  idl_union_t *u;
  const char *str;

  str = T("char", "baz") U("baz");
  ret = idl_parse_string(str, 0u, &tree);
  CU_ASSERT_EQUAL_FATAL(ret, IDL_RETCODE_OK);
  CU_ASSERT_PTR_NOT_NULL_FATAL(tree);
  t = (idl_typedef_t *)tree->root;
  CU_ASSERT(idl_is_typedef(t));
  u = idl_next(t);
  CU_ASSERT_FATAL(idl_is_union(u));
  CU_ASSERT_PTR_EQUAL(t, u->switch_type_spec);
  idl_delete_tree(tree);

  str = M("foo", T("char", "baz") U("baz"));
  ret = idl_parse_string(str, 0u, &tree);
  CU_ASSERT_EQUAL_FATAL(ret, IDL_RETCODE_OK);
  CU_ASSERT_PTR_NOT_NULL_FATAL(tree);
  m = (idl_module_t *)tree->root;
  CU_ASSERT(idl_is_module(m));
  t = (idl_typedef_t *)m->definitions;
  CU_ASSERT(idl_is_typedef(t));
  u = idl_next(t);
  CU_ASSERT(idl_is_union(u));
  CU_ASSERT_PTR_EQUAL(t, u->switch_type_spec);
  idl_delete_tree(tree);

  str = M("foo", T("char", "baz")) M("bar", U("foo::baz"));
  ret = idl_parse_string(str, 0u, &tree);
  CU_ASSERT_EQUAL_FATAL(ret, IDL_RETCODE_OK);
  CU_ASSERT_PTR_NOT_NULL_FATAL(tree);
  m = (idl_module_t *)tree->root;
  CU_ASSERT(idl_is_module(m));
  t = (idl_typedef_t *)m->definitions;
  CU_ASSERT(idl_is_typedef(t));
  m = idl_next(m);
  CU_ASSERT(idl_is_module(m));
  u = (idl_union_t *)m->definitions;
  CU_ASSERT(idl_is_union(u));
  CU_ASSERT_PTR_EQUAL(t, u->switch_type_spec);
  idl_delete_tree(tree);
}

CU_TheoryDataPoints(idl_union, bad_switch_types) = {
  CU_DataPoints(const char *,
    S("baz") U("baz"),
    U("baz"),
    M("foo", T("float", "baz")) M("bar", U("foo::baz"))),
  CU_DataPoints(idl_retcode_t,
    IDL_RETCODE_SEMANTIC_ERROR,
    IDL_RETCODE_SEMANTIC_ERROR,
    IDL_RETCODE_SEMANTIC_ERROR)
};

CU_Theory((const char *str, idl_retcode_t expret), idl_union, bad_switch_types)
{
  idl_retcode_t ret;
  idl_tree_t *tree = NULL;

  ret = idl_parse_string(str, 0u, &tree);
  CU_ASSERT_EQUAL(ret, expret);
  CU_ASSERT_PTR_NULL(tree);
  idl_delete_tree(tree);
}