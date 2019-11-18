/**
 * BD ADD:
 */
library track_route_constructor_locations;

import 'package:kernel/ast.dart';
import 'package:kernel/class_hierarchy.dart';
import 'package:meta/meta.dart';
import 'package:vm/frontend_server.dart' show ProgramTransformer;

const String _creationLocationParameterName = 'settings';

class _RouteCallSiteTransformer extends Transformer {
  final ClassHierarchy _hierarchy;
  final Class _routeClass;
  final Class _routeSettingsClass;

  const _RouteCallSiteTransformer(this._hierarchy, this._routeClass,
      this._routeSettingsClass,);

  @override
  StaticInvocation visitStaticInvocation(StaticInvocation node) {
    node.transformChildren(this);
    final Procedure target = node.target;
    if (!target.isFactory) {
      return node;
    }
    final Class constructedClass = target.enclosingClass;
    if (!_isSubclassOfRoute(constructedClass)) {
      return node;
    }

    _addLocationArgument(node, target.function, constructedClass);
    return node;
  }

  @override
  ConstructorInvocation visitConstructorInvocation(ConstructorInvocation node) {
    node.transformChildren(this);

    final Constructor constructor = node.target;
    final Class constructedClass = constructor.enclosingClass;
    if (!_isSubclassOfRoute(constructedClass)) {
      return node;
    }

    _addLocationArgument(node, constructor.function, constructedClass);
    return node;
  }

  void _addLocationArgument(InvocationExpression node, FunctionNode function,
      Class constructedClass) {
    final Location location = node.location;
    final List<NamedExpression> arguments = <NamedExpression>[
      new NamedExpression(
          'name', new StringLiteral(location.toString())),
    ];
    Expression newRouteSettings = new ConstructorInvocation(
      _routeSettingsClass.constructors.first,
      new Arguments(<Expression>[], named: arguments),
      isConst: true,
    );
    _maybeAddCreationLocationArgument(
      node.arguments,
      function,
      newRouteSettings,
    );
  }

  /// Add the creation location to the arguments list if possible.
  void _maybeAddCreationLocationArgument(Arguments arguments,
      FunctionNode function,
      Expression creationLocation,) {
    if (_hasNamedArgument(arguments, _creationLocationParameterName)) {
      return;
    }
    if (!_hasNamedParameter(function, _creationLocationParameterName)) {
      return;
    }

    final NamedExpression namedArgument =
    new NamedExpression(_creationLocationParameterName, creationLocation);
    namedArgument.parent = arguments;
    arguments.named.add(namedArgument);
  }

  bool _hasNamedParameter(FunctionNode function, String name) {
    return function.namedParameters
        .any((VariableDeclaration parameter) => parameter.name == name);
  }

  bool _hasNamedArgument(Arguments arguments, String argumentName) {
    return arguments.named
        .any((NamedExpression argument) => argument.name == argumentName);
  }

  bool _isSubclassOfRoute(Class clazz) {
    return _hierarchy.isSubclassOf(clazz, _routeClass);
  }
}


/// 构建Route时，如果没有传递RouteSettings参数，主动创建一个RouteSettings，传给Route
/// 保证通过Route能够获取到页面命名之类的信息
/// RouteSettings的name参数(即页面命名)为Route的创建位置
class RouteCreatorTracker implements ProgramTransformer {
  final ProgramTransformer _nextTransformer;
  Class _routeClass;
  Class _routeSettinsClass;

  RouteCreatorTracker({ProgramTransformer nextTransformer})
      :_nextTransformer=nextTransformer;

  /// Transform the given [program].
  ///
  /// It is safe to call this method on a delta program generated as part of
  /// performing a hot reload.
  @override
  void transform(Component program) {
    final List<Library> libraries = program.libraries;
    if (libraries.isEmpty) {
      return;
    }

    _resolveFlutterClasses(libraries);

    if (_routeClass == null || _routeSettinsClass == null) {
      // This application doesn't actually use the package:flutter library.
      return;
    }
    ClassHierarchy hierarchy = new ClassHierarchy(
      _computeFullProgram(program),
      onAmbiguousSupertypes: (Class cls, Supertype a, Supertype b) {},
    );

    // Transform call sites to pass the location parameter.
    final _RouteCallSiteTransformer callsiteTransformer = new _RouteCallSiteTransformer(
      hierarchy,
      _routeClass,
      _routeSettinsClass,
    );

    for (Library library in libraries) {
      if (library.isExternal) {
        continue;
      }
      library.transformChildren(callsiteTransformer);
    }
    if (_nextTransformer != null) {
      _nextTransformer.transform(program);
    }
  }

  void _resolveFlutterClasses(Iterable<Library> libraries) {
    for (Library library in libraries) {
      final Uri importUri = library.importUri;
      if (!library.isExternal &&
          importUri != null &&
          importUri.scheme == 'package') {
        if (importUri.path == 'flutter/src/widgets/navigator.dart') {
          for (Class class_ in library.classes) {
            if (class_.name == 'Route') {
              _routeClass = class_;
            }
            if (class_.name == 'RouteSettings') {
              _routeSettinsClass = class_;
            }
          }
        }
      }
    }
  }

  Component _computeFullProgram(Component deltaProgram) {
    final Set<Library> libraries = new Set<Library>();
    final List<Library> workList = <Library>[];
    for (Library library in deltaProgram.libraries) {
      if (libraries.add(library)) {
        workList.add(library);
      }
    }
    while (workList.isNotEmpty) {
      final Library library = workList.removeLast();
      for (LibraryDependency dependency in library.dependencies) {
        if (libraries.add(dependency.targetLibrary)) {
          workList.add(dependency.targetLibrary);
        }
      }
    }
    return new Component()
      ..libraries.addAll(libraries);
  }
}
