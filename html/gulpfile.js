var gulp = require('gulp'),
    jshint = require('gulp-jshint'),
    gulpif = require('gulp-if'),
    concat = require('gulp-concat'),
    uglify = require('gulp-uglify'),
    cleanCSS = require('gulp-clean-css'),
    removeCode = require('gulp-remove-code'),
    imagemin = require('gulp-imagemin'),
    merge = require('merge-stream'),
    del = require('del'),
    zip = require('gulp-zip'),
    gzip = require('gulp-gzip'),
    htmlmin = require('gulp-htmlmin'),
    replace = require('gulp-replace'),
    fs = require('fs'),
    smoosher = require('gulp-smoosher'),
    base64 = require('gulp-base64'),
    size = require('gulp-filesize');


function clean() {
    return del(['dist']);
    
}

function clean2() {
    return del(['dist/js', 'dist/css']);
}
function lint() {
    return gulp.src('js/**/app.js')
        .pipe(jshint())
        .pipe(jshint.reporter('default'));
}

function Copytest() {
    return merge(
        gulp.src(['index.html'])
        .pipe(removeCode({production: false}))
        .pipe(removeCode({cleanheader: true}))
        .pipe(gulp.dest('dist')),
        gulp.src(['images/*.png'])
            .pipe(gulp.dest('dist/images'))
    )
}

function Copy() {
    return merge(
        gulp.src(['index.html'])
        .pipe(removeCode({production: true}))
        .pipe(removeCode({cleanheader: true}))
        .pipe(gulp.dest('dist')),
        gulp.src(['images/*.*'])
            .pipe(gulp.dest('dist/images'))
    )
}

function concatApptest() {
    return merge(
        gulp.src([ 'js/*.js'])
            .pipe(concat('app.js'))
            .pipe(removeCode({production: false}))
            .pipe(removeCode({cleanheader: true}))
            .pipe(gulp.dest('./dist/js')),

        gulp.src([ 'css/*.css'])
            .pipe(concat('style.css'))
            .pipe(gulp.dest('./dist/css/'))
    )
}

function concatApp() {
    return merge(
        gulp.src([ 'js/*.js'])
            .pipe(concat('app.js'))
            .pipe(removeCode({production: true}))
            .pipe(removeCode({cleanheader: true}))
            .pipe(gulp.dest('./dist/js')),

        gulp.src([ 'css/*.css'])
            .pipe(concat('style.css'))
            .pipe(gulp.dest('./dist/css/'))
    )
}

function replaceSVG() {
    return gulp.src('dist/index.html')
        .pipe(replace(/<!-- replaceSVG -->(.*?)<!-- \/replaceSVG -->/g, function (match, p1) {
            return fs.readFileSync('dist/images/jogdial.svg', 'utf8');
        }))
        .pipe(gulp.dest('dist'))
}

function replacePNG() {
    return gulp.src('dist/index.html')
        .pipe(replace(/<!-- replacePNG -->(.*?)<!-- \/replacePNG -->/g, function (match, p1) {
            return fs.readFileSync('dist/images/m3d-logo.png', 'utf8');
        }))
        .pipe(gulp.dest('dist'))
}

function minifyApp() {
    return merge(
        gulp.src(['dist/js/app.js'])
            //.pipe(uglify({mangle: true}))
            .pipe(gulp.dest('./dist/js/')),

        gulp.src('dist/css/style.css')
            .pipe(cleanCSS({debug: true}, function(details) {
                console.log(details.name + ': ' + details.stats.originalSize);
                console.log(details.name + ': ' + details.stats.minifiedSize);
            }))
            .pipe(gulp.dest('./dist/css/')),

        gulp.src('dist/index.html')
            .pipe(htmlmin({collapseWhitespace: true, minifyCSS: true}))
            .pipe(gulp.dest('dist'))
    )
}

function smoosh() {
    return gulp.src('dist/index.html')
        .pipe(smoosher())
        .pipe(gulp.dest('dist'))
}

function compress() {
    return gulp.src('dist/index.html')
        .pipe(gzip({ gzipOptions: { level: 9} }))
        .pipe(gulp.dest('.'))
        .pipe(size());
}


gulp.task(clean);
gulp.task(lint);
gulp.task(Copy);
gulp.task(Copytest);
gulp.task(replaceSVG);
gulp.task(replacePNG);
gulp.task(concatApp);
gulp.task(concatApptest);
gulp.task(minifyApp);
gulp.task(smoosh);
gulp.task(clean2);

var defaultSeries = gulp.series(clean,  lint, Copy, concatApp);
//var packageSeries = gulp.series(clean,  lint, Copy, concatApp, minifyApp, smoosh, compress);
var deplySeries = gulp.series(clean,  lint, Copy, concatApp, replaceSVG, minifyApp, smoosh);
var packageSeries = gulp.series(clean, lint, Copy, concatApp, replaceSVG, minifyApp, smoosh, compress, clean2);
var package2Series = gulp.series(clean,  lint, Copy, concatApp, replaceSVG, smoosh);
var package2testSeries = gulp.series(clean,  lint, Copytest, concatApptest, smoosh);

gulp.task('default', defaultSeries);
gulp.task('deploy', deplySeries);
gulp.task('package', packageSeries);
gulp.task('package2', package2Series);
gulp.task('package2test', package2testSeries);

